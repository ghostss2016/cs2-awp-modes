# vim: set sts=2 ts=8 sw=2 tw=99 et ft=python:
import os

for sdk_target in MMSPlugin.sdk_targets:
  sdk = sdk_target.sdk
  cxx = sdk_target.cxx

  binary = MMSPlugin.HL2Library(builder, cxx, MMSPlugin.plugin_name, sdk)

  # eiface.h (engine 26 / build 2000872 + PR#396) now force-includes
  # network_connection.pb.h. Generate it from the SDK proto so the include
  # resolves; the ambuild Protoc tool auto-adds the build folder to includes.
  binary.custom = [builder.tools.Protoc(protoc = sdk_target.protoc, sources = [
    os.path.join(sdk['path'], 'common', 'network_connection.proto')
  ])]

  schema_dir = os.path.join(builder.sourcePath, '..', 'SchemaEntity')

  binary.sources += [
    'src/glibc_compat.c',
    'src/main.cpp',
    'src/config.cpp',
    'src/weapon_model.cpp',
    'src/round_type.cpp',
    'src/arena_player.cpp',
    'src/arena.cpp',
    'src/arenas.cpp',
    'src/arena_finder.cpp',
    'src/commands.cpp',
    'src/events.cpp',
    'src/database.cpp',
    'src/menus.cpp',
    'src/entitysystem_minimal.cpp',
    os.path.join(schema_dir, 'schemasystem.cpp'),
    os.path.join(schema_dir, 'module.cpp'),
  ]

  binary.compiler.includes += [
    os.path.join(MMSPlugin.mms_root, 'core'),
    os.path.join(MMSPlugin.mms_root, 'core', 'sourcehook'),
    os.path.join(builder.sourcePath, '..', 'cs2-menus-new', 'include'),
    schema_dir,
    os.path.join(builder.sourcePath, '..', 'sql_mm', 'src', 'public'),
  ]

  binary.compiler.cxxflags += [
    '-Wno-sign-compare',
    '-Wno-unused-variable',
  ]

  MMSPlugin.binaries += [builder.Add(binary)]
