# Weapon Wheel Pause

A small native OpenRAC mod for Ratchet & Clank 1 that pauses gameplay while the in-game weapon wheel is open, while keeping the wheel itself responsive.

## Behavior

- Freezes Ratchet while Quick Select is open.
- Freezes world moby updates when the OpenRAC moby-update gate is available.
- Keeps the weapon wheel responsive so a weapon can still be selected normally.
- Falls back safely if another mod already owns the world-update gate.
- Supports RAC1 PAL and NTSC-U.

## Multiplayer note

The OpenRAC moby-update gate is exclusive. If another mod, such as a multiplayer implementation, already owns it, Weapon Wheel Pause still freezes Ratchet but cannot independently freeze all world mobies.

## Install

Use the OpenRAC launcher Mods tab with this repository/release, or install the matching release ZIP.

## Requirements

- OpenRAC mod API 1.12 or newer.

## License

MIT. See `LICENSE`.
