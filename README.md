# Weapon Wheel Pause

Pause **Ratchet & Clank (2002)** in **OpenRAC** while choosing a weapon.

**Weapon Wheel Pause was created by Moritz Wallinger and glawks.** Version 2.0.0, by Moritz
Wallinger, glawks and Henning Berge, keeps the mod and its idea and replaces its implementation with
**Going Commando's own weapon quick select**, copied from Going Commando's decompilation and run on
Ratchet & Clank:

- **Hold Triangle**: the world holds (Going Commando's pause, no freeze dialog, no dimming) and the
  quick select opens. Pick a weapon with the stick (Going Commando's sectors, hysteresis and
  snapping) or the d-pad; Going Commando's ammo and name text shows under the ring. Let go of
  Triangle and the weapon is equipped.
- **Double tap Triangle**: the quick swap to your previous weapon, as in Going Commando, without
  opening the quick select.
- The ring, its icons, the slots and their neighbours, the names and the ammo format are Ratchet &
  Clank's own, read from your game.

**License**: 2.0.0 is GPL-3.0-only, because it includes code copied from the GPL-3.0-only
decompilation. The original MIT notice of Moritz Wallinger and glawks is kept in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## Installation

Install **Weapon Wheel Pause** using OpenRAC's **Mods** tab, or download the mod ZIP from the
[GitHub Releases page](https://github.com/momo1212221/weapon-wheel-pause/releases) and add it there.
Ratchet & Clank PAL and NTSC-U, on Windows x86_64, Linux x86_64/aarch64, macOS arm64/x86_64 and
Android ARM64.

## Settings

- **Pause while open** (on): Going Commando's option. On, the world holds while the quick select is
  open. Off, it opens over the running game, from Ratchet & Clank's own Triangle, as Going Commando
  does with the option off.
- **Hold before pausing** (A little longer): how long Triangle is held before the pause.
  - *Going Commando*: its own 4 frames (a 15th of a second).
  - *A little longer*, **the default, longer than Going Commando's**: 8 frames. A natural tap is
    often 4 or 5 frames long, so with Going Commando's 4, running and double-tap swapping pause the
    game for a moment before the swap; at 8 they usually do not.
  - *Longer*: 12 frames.

  On PAL each is scaled to 50 ticks a second (3, 7, 10).
- **Click sound** (off): a sound as the selection moves, where Going Commando clicks. Ratchet &
  Clank's quick select is silent and has no UI click: on, it plays a stand-in (the sound a help
  message opens with), not Going Commando's.

## Requirements

**OpenRAC with mod interface 1.39 and three new hook points**, `rac1.level.update_play`,
`rac1.freeze.update` and `rac1.freeze.draw`, added by
[OpenRAC pull request #863](https://github.com/OpenRAC/OpenRAC/pull/863) and in OpenRAC's develop,
but not in a release yet. A game without them refuses the mod and says why. Until a release has
them, `sdk.lock` pins the SDK of OpenRAC's develop commit that added them (`be0351ce`).

Ratchet & Clank PAL and NTSC-U.

## Differences from Going Commando

- **No black bars**: Going Commando draws black bars at the top and bottom while the quick select
  is open. They are left out on purpose: Ratchet & Clank's letterbox changes its viewport, so the
  frozen world would zoom.
- **A longer hold before the pause** by default (above); "Going Commando" restores its own.
- **Where Going Commando's code is silent, Ratchet & Clank's behaviour is kept**, never a guess at
  Going Commando's:
  - the quick select's place on the screen (Going Commando's is in its disc's HUD data);
  - the ring, icons and selection marker (Going Commando's are its resident HUD's art): Ratchet &
    Clank's sprites, layout, faded unselected icons and pulsing marker, at Going Commando's fades;
  - the weapon names (the gold name for a gold weapon) and the ammo format;
  - the quick swap's rule (to the weapon held before, or with the wrench in hand the last weapon)
    and the usage statistics;
  - the silence (see Click sound);
  - no class preload, scripted-sequence hold or hit-streak input block (Ratchet & Clank has none).

## Compatibility

- **Pause Weapon Wheel** (`pause-weapon-wheel`, glawks' earlier mod): conflicts (both replace the
  quick select and pause the game). The manifest says so.
- **Gadget Quick Use**: untested together. It reads Triangle at pads, faucets and the Swingshot. A
  press it takes out of the pad report never reaches this mod; a press it leaves in may do both.
- **Enhanced Movement**: while the paused quick select is open the right-stick camera does not
  move (the world is held); with the pause off it does, as in Going Commando.
- **Going Commando Magneboots**, **Rumble**: no clash expected (a pick goes through the game's own
  change of weapon; the rumble goes quiet while the world is held).

## The log

In the game's log (`openrac-rac1-*.log`, lines `[mod weapon-wheel-pause]`): once at load and each
level, whether the hook points and the quick select's functions were found; then at most one line
per Triangle press (why it did not open, or the hold's trace: asked, counted, opened or cancelled)
and one per double-tap swap (or why a second tap did not swap). A warning if the pause ever had to
be ended by its watchdog.

## How it works

The quick select is Going Commando's, copied from its decompilation
([rac2-decomp](https://github.com/OpenRAC/rac2-decomp), GPL-3.0, at `4ea93cea`) into `src/gc/`,
unchanged but for the lines marked `(gc-weapon-wheel)` (the port's own marker) ([THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)
lists every file and line):

- `weapon_wheel_update_select`, `weapon_wheel_draw` (its text; Ratchet & Clank's ring in place of
  Going Commando's), `menu_init_panel_layout`, `weapon_menu_init_slots`: the quick select itself;
- the pause: `level_update_game_frame_input` (the gate: a Triangle press asks for the HUD-open
  request, `hud_request_open`; `level_update_countdown_state_machine` counts it while Triangle is
  held; then `request_begin_slot(4, 1, 7)`), `UpdateModalState` (mode 7's four phases),
  `menu_set_mode`, `menu_end`, `menu_set_state_word_three`, `menu_advance_state_in_mode_seven`,
  `menu_is_state_seven`, `gfx_render_frame_extra_layers`, `func_0029CE38`, `hud_clear_flag_a/_b`;
- the input: `PadPressedWithin`, the press window (`hero_update_hit_streak_effect`'s first block)
  and Triangle's double tap (`hero_update_button_hold_timers`).

A translation layer (`src/gc/rac1_shim.*`, `src/gc/shim/common.h`) runs it on Ratchet & Clank, whose
quick select is Going Commando's ancestor, record for record: the same 0x90-byte HUD record and
pad, read straight from game memory; copies of what Ratchet & Clank keeps at other offsets (the
hero's item, pending item, mode and timers; the usage entry; a weapon table filled from
`item_defs` and `item_price_table`); Ratchet & Clank's own functions by name for Going Commando's
(each polyfill says what differs). Going Commando's game mode 4 with modal mode 7 runs in Ratchet &
Clank's game mode 4, the freeze mode, its ancestor, with a freeze kind of the mod's own; the three
new hook points let the gate run before the level's tick (skipping it when the pause opens, as
Going Commando returns before its world update), and the freeze's tick and picture run Going
Commando's. Going Commando counts 60 frames a second: on PAL the copied frame counts are scaled as
its own PAL build scales them, `(n * 5 + 2) / 6`.

Read from Going Commando's own program (USA v2.00), where the decompilation's C is the executable's
and the game runs a level's copy: mode 7 runs the double tap's timers (`hero_update_button_hold_timers`
where the C calls an empty stub), and `menu_set_state_word_three` sets mode 7's timer, which keeps
the pause as long as the quick select is open. [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)
gives the addresses.

The mod's own parts, marked: the gate's hero conditions are Ratchet & Clank's (the item slot not
locked, no timed HUD, the item locks, on foot); the gate also checks the pause can be entered; the
press window is long enough for a longer "Hold before pausing"; watchdogs end the pause if anything
goes wrong (a freeze with no quick select, missing functions, or a port whose freeze tick does not
run).

## Build

```sh
python3 tools/fetch_sdk.py          # the SDK sdk.lock pins, into .sdk/
cmake -S . -B build -G Ninja
cmake --build build && ctest --test-dir build
python3 .sdk/openrac-mod-sdk-*/tools/package_mod.py build/package/weapon_wheel_pause
```

The test (`tests/weapon_wheel_test.cpp`) runs the copied code and the layer on a fake game: the
selection, the d-pad, the commit, the draw, the pause from the hold to the release, the gate's
conditions, the taps, double taps and holds at each "Hold before pausing", wheel picks followed by
double taps, the 50/60 Hz scaling, each level's functions, and the watchdogs.

## Credits

- The original Weapon Wheel Pause, its idea and its earlier versions (1.x): Moritz Wallinger and
  glawks.
- The Going Commando quick select port (2.0.0): Henning Berge.

- Going Commando's code: [rac2-decomp](https://github.com/OpenRAC/rac2-decomp) (Kryštof "Lynder063"
  Malinda and its contributors; earlier llesieur99).
- Ratchet & Clank's: [OpenRAC](https://github.com/OpenRAC/OpenRAC)'s decompilations
  (`games/rac1/decomp`, after Lombyte's project, and the NTSC decomp), and the OpenRAC mod SDK.

## License

GNU General Public License version 3 only ([LICENSE](LICENSE)). The files copied from rac2-decomp
are GPL-3.0-only, as that project gives no later version; the files written for the port are
GPL-3.0-or-later; as a whole the mod is GPL-3.0-only. Versions 1.x were MIT, by Moritz Wallinger and
glawks: their notice is kept in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) names the source, its commit, the files and lines
copied, and its earlier MIT notice; the package carries it as `LICENSE.THIRD_PARTY_NOTICES.md`.

Ratchet & Clank and Ratchet & Clank: Going Commando are trademarks of Sony Interactive
Entertainment, developed by Insomniac Games. This mod is not affiliated with or endorsed by
either, and contains none of their code or assets.
