"""Compiler-free migration policy; native KHook ABI and gameplay are separate gates.
Handler fingerprints originate at the published pre-migration source commit.
Only comments, preprocessor guards and legacy Ignore routing are normalized.
"""
from pathlib import Path
import hashlib
import re
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
OWNER = "CS2AWPModes"
SOURCE = "src/main.cpp"
HEADER = "include/main.h"
HANDLERS = {"Hook_GameFrame":"819f3b62720aa0773ddc115b880c3b2973ea921e6072fa624178800ce4e9f339","Hook_ClientPutInServer":"9dfc4bf66a3d087b54c13516410d8d8a9ab724d4c717a8e340ca1c51e61dd392","Hook_ClientDisconnect":"2b414b4559f72bb303c190abf2acb3de20cfa4420ac414bc9e54216b323f4f8b"}
HOOKS = [["frameHook_","FrameHook","IServerGameDLL","GameFrame","Api18Frame","post","g_pSource2Server"],["putHook_","PutHook","IServerGameClients","ClientPutInServer","Api18Put","post","g_pSource2GameClients"],["disconnectHook_","DisconnectHook","IServerGameClients","ClientDisconnect","Api18Disconnect","post","g_pSource2GameClients"]]


def body(text, name):
    match = re.search(r'\b(?:void|bool)\s+' + re.escape(OWNER + '::' + name) + r'\([^;{]*\)\s*\{', text)
    if not match: raise AssertionError('Missing handler: ' + name)
    start, depth = match.end() - 1, 0
    for i in range(start, len(text)):
        depth += (text[i] == '{') - (text[i] == '}')
        if depth == 0: return text[start:i + 1]
    raise AssertionError('Unclosed handler: ' + name)


def modern(text):
    stack, output = [], []
    for line in text.splitlines():
        directive = re.match(r'\s*#\s*(if|ifdef|ifndef|else|elif|endif)\b(.*)', line)
        if directive:
            kind, condition = directive.groups()
            if kind in ('if', 'ifdef', 'ifndef'):
                value = None
                if re.fullmatch(r'\s*METAMOD_PLAPI_VERSION\s*<\s*18\s*', condition): value = False
                if re.fullmatch(r'\s*METAMOD_PLAPI_VERSION\s*>=\s*18\s*', condition): value = True
                stack.append(value)
            elif kind == 'else' and stack[-1] is not None: stack[-1] = not stack[-1]
            elif kind == 'endif': stack.pop()
            elif kind == 'elif' and (stack[-1] is not None or 'METAMOD_PLAPI_VERSION' in condition):
                raise AssertionError('Unknown API branch')
        elif False not in stack: output.append(line)
    if stack: raise AssertionError('Unbalanced preprocessor')
    return '\n'.join(output)


class Migration(unittest.TestCase):
    def test_handlers_preserved(self):
        source = (ROOT / SOURCE).read_text()
        for name, digest in HANDLERS.items():
            with self.subTest(handler=name):
                text = body(source, name)
                text = re.sub(r'^\s*#.*$', '', text, flags=re.M)
                text = re.sub(r'RETURN_META\(MRES_IGNORED\);', '', text)
                text = re.sub(r'//[^\n]*|/\*.*?\*/', '', text, flags=re.S)
                text = re.sub(r'\s+', '', text)
                self.assertEqual(hashlib.sha256(text.encode()).hexdigest(), digest)

    def test_modern_sourcehook_removed(self):
        for path in [SOURCE, HEADER]:
            text = modern((ROOT / path).read_text())
            self.assertNotIn('sh_vector.h', text)
            self.assertNotIn('KHook::HookType', text)
            self.assertIsNone(re.search(r'\b(?:SH_\w+|RETURN_META\w*)\s*\(', text))

    def test_owned_phase_registration_rollback_and_removal(self):
        source = modern((ROOT / SOURCE).read_text())
        header = modern((ROOT / HEADER).read_text())
        load, unload = body(source, 'Load'), body(source, 'Unload')
        for member, kind, interface, method, callback, phase, target in HOOKS:
            with self.subTest(hook=method):
                pre, post = ('&' + OWNER + '::' + callback, 'nullptr') if phase == 'pre' else ('nullptr', '&' + OWNER + '::' + callback)
                self.assertIn('std::unique_ptr<' + kind + '> ' + member, source + header)
                self.assertIn('std::make_unique<' + kind + '>(&' + interface + '::' + method + ',this,' + pre + ',' + post + ')', load)
                self.assertIn('!' + member + '->AddInstance(' + target + ')', load)
                self.assertLess(load.index(member + '.reset()'), load.index('return false;'))
                self.assertIn(member + '.reset()', unload)
        if OWNER == 'VIPGraffiti':
            self.assertLess(unload.index('messageHook_.reset()'), unload.index('g_web.reset()'))
            self.assertIn('ProcessClientSvcUserMessage(a,b,c,d)?KHook::Action::Supersede:KHook::Action::Ignore', header)


if __name__ == '__main__':
    result = unittest.TextTestRunner().run(unittest.defaultTestLoader.loadTestsFromTestCase(Migration))
    if not result.wasSuccessful(): raise SystemExit(1)
    if len(sys.argv) == 2: Path(sys.argv[1]).write_text('passed\n')
