#!/usr/bin/env bash
# ci/build.sh — build a CS2 MM:S plugin against pinned SDK, enforce cleanliness gates.
# Env: HL2SDK (=.../hl2sdk-cs2), MMS (=.../metamod-source). Writes so_path to $GITHUB_OUTPUT.
set -u
MAN="$MMS/hl2sdk-manifests"

# force lld + link libdl/libpthread: Debian11=glibc2.31 держит dlopen/pthread_* в отдельных libdl/libpthread
# (на билд-машине glibc2.34+ они в libc, потому локальный AMBuilder их не линковал). Обёртка дописывает -ldl -lpthread.
LLD=""; for L in ld.lld-17 ld.lld lld-17 lld; do command -v "$L" >/dev/null && { LLD="$(command -v "$L")"; break; }; done
if [ -n "$LLD" ]; then
  printf '#!/bin/sh\nexec %s "$@" -ldl -lpthread\n' "$LLD" > /usr/local/bin/ld
  chmod +x /usr/local/bin/ld
  echo "linker=lld ($LLD) +(-ldl -lpthread)"
fi

# 1. locate configure.py (root | source/ | src/)
SRCDIR=""
for d in "." "source" "src"; do
  [ -f "$d/configure.py" ] && { SRCDIR="$d"; break; }
done
[ -z "$SRCDIR" ] && { echo "::error::configure.py not found (root/source/src)"; exit 1; }
echo "srcdir=$SRCDIR"
cd "$SRCDIR"

# shared dep: плагины компилируют ../SchemaEntity (наш форк ghostss2016/SchemaEntity — с GiveNamedItem 5-арг фиксом).
# Провижн как сосед sourcePath. rm -rf: репо может содержать пустой остаток SchemaEntity (git откажется клонировать поверх).
if grep -rqsE "SchemaEntity" . 2>/dev/null; then   # любой файл (не только AMBuild — напр. graffiti ссылается из .cpp)
  SE="$(mktemp -d)"; rm -rf "$SE"
  git clone --depth 1 https://github.com/ghostss2016/SchemaEntity.git "$SE" 2>/dev/null \
    || git clone --depth 1 https://github.com/Pisex/SchemaEntity.git "$SE"
  # разные плагины ждут SchemaEntity по РАЗНЫМ относительным путям (AMBuilder builder.sourcePath +'..'):
  #   обычные: ../SchemaEntity ; admin/chat-субмодули: ../../samples/SchemaEntity
  for dst in ../SchemaEntity ../../samples/SchemaEntity; do
    rm -rf "$dst"; mkdir -p "$(dirname "$dst")"; cp -r "$SE" "$dst" 2>/dev/null
  done
  rm -rf "$SE"
  echo "provisioned SchemaEntity (../ + ../../samples/)"
fi

# unify shared interface headers: menus.h (IUtilsApi) разъехался по плагинам (40/41/43 слота) -> vtable mismatch -> краш смены карты.
# Канон = ghostss2016/cs2-shared-headers (43 слота, = деплоенная utils.so). Перезаписываем локальные копии единым источником.
SH="$(mktemp -d)"
if git clone --depth 1 -q https://github.com/ghostss2016/cs2-shared-headers.git "$SH" 2>/dev/null; then
  for hdr in menus.h sql_mm.h mysql_mm.h; do
    [ -f "$SH/$hdr" ] || continue
    # перезаписываем ТОЛЬКО консюмер-копии в include/ (НЕ src/ — там определения-первоисточники, напр. sql_mm/src/sql_mm.h)
    find . -path '*/include/*' -name "$hdr" ! -path './.git/*' -exec cp "$SH/$hdr" {} \; 2>/dev/null
    if grep -rqsE "#include[ \t]*[<\"]$hdr" . 2>/dev/null; then                    # инклудится но отсутствует -> добавить
      [ -d include ] && [ ! -f "include/$hdr" ] && cp "$SH/$hdr" "include/$hdr"
    fi
  done
  echo "unified shared headers: menus.h(IUtilsApi 43) sql_mm.h mysql_mm.h ($(cd "$SH" && git rev-parse --short HEAD))"
fi
rm -rf "$SH"

rm -rf build && mkdir build && cd build

# 2. try configure arg-sets until one succeeds (accepted args vary per plugin)
CONFIGURED=0
ARGSETS=(
  "--enable-optimize --symbol-files --sdks cs2 --mms_path $MMS --hl2sdk-root $HL2SDK --hl2sdk-manifests $MAN"
  "--enable-optimize --sdks=cs2 --targets=x86_64 --mms_path=$MMS --hl2sdk-root=$HL2SDK --hl2sdk-manifests=$MAN"
  "--hl2sdk-root=$HL2SDK --mms_path=$MMS"
  "--hl2sdk-root $HL2SDK --mms_path $MMS"
)
for A in "${ARGSETS[@]}"; do
  echo "+ configure $A"
  if CC=clang CXX=clang++ python3 ../configure.py $A >configure.log 2>&1; then CONFIGURED=1; echo "configured with: $A"; break; fi
done
[ "$CONFIGURED" -ne 1 ] && { echo "::error::all configure arg-sets failed"; tail -20 configure.log; exit 1; }

# 3. build
if ! ambuild >ambuild.log 2>&1; then
  echo "::error::ambuild failed"
  echo "----- configure.log (tail) -----"; tail -40 configure.log
  echo "----- .ambuild2 present? -----"; ls -la .ambuild2 2>&1 | head
  echo "----- ambuild.log (tail) -----"; grep -aiE 'error:|undefined|ld:|No such|Errno' ambuild.log | head -20
  exit 1
fi

# 4. find produced .so (exclude libtier0.so)
SO="$(find . -path '*package*' -name '*.so' ! -name 'libtier0.so' | head -1)"
[ -z "$SO" ] && SO="$(find . -name '*.so' ! -name 'libtier0.so' ! -name 'libserver.so' | head -1)"
[ -z "$SO" ] && { echo "::error::no .so produced"; exit 1; }
SO="$(readlink -f "$SO")"
echo "built: $SO"

# 5. cleanliness gates
BAD=0
if nm -D -u "$SO" 2>/dev/null | grep -q g_bUpdateStringTokenDatabase; then
  echo "::error::imports undefined g_bUpdateStringTokenDatabase (stale hl2sdk)"; BAD=1
fi
HIGH="$(objdump -T "$SO" 2>/dev/null | grep -oE 'GLIBC_2\.(3[3-9]|[4-9][0-9])' | sort -u | tr '\n' ' ')"
if [ -n "$HIGH" ]; then
  echo "::error::requires too-new glibc symbols ($HIGH) — target is Steam Runtime glibc 2.31"; BAD=1
fi
[ "$BAD" -ne 0 ] && exit 1

echo "cleanliness OK"
[ -n "${GITHUB_OUTPUT:-}" ] && echo "so_path=$SO" >> "$GITHUB_OUTPUT"
echo "=== BUILD OK: $SO ==="
