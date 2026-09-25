# CS2 AWP Modes (MetaMod:Source)

Native MetaMod plugin for an AWP-focused server. The initial profile is a 1v1 ladder/arena mode with a strict AWP round catalogue. It keeps the existing arena queue, spawn detection, rank movement, challenge flow and player statistics, but does not expose the generic rifle/SMG/shotgun rounds.

The mode is controlled by `deploy/cfg/cs2-awp-modes.cfg`:

- `Mode = awp_arena` enables AWP-only arena rounds.
- `Mode = awp_rotation` enables AWP plus the explicitly enabled Scout, Deagle and Knife rounds.
- `AllowScout`, `AllowDeagle` and `AllowKnife` are allow-list switches. Existing database preferences cannot re-enable a disabled weapon type.
- Armor, helmet and round defaults are set in the same config.

This repository is the first implementation slice for the fleet AWP server. FFA/Deathmatch, Retake and movement variants remain separate mode profiles to be added only after their event and spawn isolation is implemented and tested; they are not silently advertised as active here.

## Build

The build uses the fleet MetaMod AMBuild pipeline and the shared `SchemaEntity`/menu headers. Run the standard CI build from the central `cs2-ci` repository after the commit is present in `repos.txt`.

The output is `cs2-awp-modes.so`; the deploy payload contains the VDF and the config template. Server-specific configs are owned by the panel and must not be overwritten as part of a binary update.

## Runtime checks

On the test server verify:

```text
meta list
meta info awp_modes
```

The plugin log prints the selected profile and the allow-list at load. For `awp_arena`, the round menu must contain AWP (and Knife only when enabled), and a rifle/SMG preference must not be offered.

## Provenance

The arena queue and native build layout are derived from the fleet's existing `cs2-arenas` MetaMod source. See `NOTICE.md` and the source history for the internal provenance record.
