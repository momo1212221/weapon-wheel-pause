/* SPDX-License-Identifier: GPL-3.0-only
 *
 * COPIED from Going Commando's decompilation: OpenRAC/rac2-decomp
 * (https://github.com/OpenRAC/rac2-decomp, GNU GPL version 3) at commit
 * 4ea93ceaf43aec055e19211d36512e847860bac2, file src/levels/shared/L00_0036F488-g8.c,
 * lines 1-81, 83-86, 93, 106-107, 112-116 and 118-120. Code of that project from before its relicensing (d46a3539, 2026-10-11) is also
 * under its MIT notice: THIRD_PARTY_NOTICES.md.
 *
 * level_update_game_frame_input (USA v1.01 func_L00_0036F488): Going Commando's game mode 0 tick,
 * cut to its head and the weapon quick select's paused open: Triangle pressed within the press
 * window opens mode 7 of game mode 4 (request_begin_slot(4, 1, 7, 0, 0)), Ratchet & Clank's freeze
 * mode here. Built with GC_SHIM_UNIT_GATE (CMakeLists.txt).
 *
 * The copied lines are unchanged. The lines marked (gc-weapon-wheel) are this mod's: the hero's view, the function
 * cut down, its result, Ratchet & Clank's conditions for the hero's part, and the window scaled. Every name the copied lines use that is not
 * defined in src/gc/ is the translation layer's (gc/shim/common.h, gc/rac1_shim.cpp): Going
 * Commando's globals and functions in Ratchet & Clank's terms.
 */
#include "common.h"

/* -G8 unit with -frac2-gcse-unsplit-edges (config/cflags.txt): retail keeps one %hi of the hero
 * and one of the pad state in saved registers and forms each block's %lo base afresh; the pad's
 * %hi is hoisted above the first branch on the gate's result. The globals below that retail reads
 * both $gp-relative (in delay slots) and absolute are MACRO_ADDR. */

typedef struct HeroObject { char pad00[0xAA]; short id; } HeroObject;
typedef struct Hero {
    char pad0000[0x123C];
    unsigned char mode;          /* 0x123C */
    char pad123D[0x1248 - 0x123D];
    int submode;                 /* 0x1248 */
    char pad124C[0x1830 - 0x124C];
    HeroObject *object;          /* 0x1830 */
    char pad1834[0x2294 - 0x1834];
    int state;                   /* 0x2294 */
    char pad2298[0x229C - 0x2298];
    int f229C;                   /* 0x229C */
    char pad22A0[0x22B4 - 0x22A0];
    unsigned char f22B4;         /* 0x22B4 */
} Hero;
typedef struct PadState { char pad000[0x1A4]; unsigned int pressed; } PadState;
typedef struct { u16 count; u16 pad; u32 maximum; u32 levels; } Statistics;

extern char g_hero_state[];
extern char g_pad_state[];

extern int g_game_l00_mode MACRO_ADDR;
extern int g_pad_press_window MACRO_ADDR;
extern int g_hero_locked_flag MACRO_ADDR;
extern int g_pad_device_idle_flag MACRO_ADDR;
extern int g_pad_hold_frame_count MACRO_ADDR;
extern int g_current_map_index MACRO_ADDR;
extern u32 g_frame_counter MACRO_ADDR;
extern int g_pickup_request_pending MACRO_ADDR;
extern Statistics g_progress_statistics[];
extern unsigned char g_language_settings[] __attribute__((sda));
extern int g_save_menu_flags __attribute__((sda));
extern int g_input_block_flag __attribute__((sda));
extern int g_hero_action_blocked_flag __attribute__((sda));
extern float g_hero_scalar_transition_value __attribute__((sda));

extern int level_get_flag_b(void);
extern int request_begin_slot(int, int, int, int, int);
/* The neighbouring stub in most programs; another function in six of them. */
extern unsigned int ref_L00_0036F488_00(void);
extern int level_is_mode_in_set(void);
extern int level_run_mode_update_zero(void);
extern int level_is_tracked_object_of_selected_class(int);
extern int level_is_special_mode_active(void);
extern void level_update_countdown_state_machine(void);
extern int PadPressedWithin(int, int, int *);
extern void hud_request_open(void);
extern int hud_is_open_done(void);
extern void hero_update_button_hold_timers(void);
extern int level_is_counter_positive(void);
extern int level_get_setting_value(void);
extern void level_set_global_setting_value(int);
extern int snd_play_from_global_bank(int, unsigned int, void *);
extern void hero_update_hit_streak_effect(void);
extern void module_l00_call_handler_with_arg_4(int);
extern void func_L00_0030C710(int);
extern void game_reset_frame_counters(void);
extern void level_run_per_frame_passes(void);
extern void level_count_active_records(void);
extern void moby_update_visible_objects(void);
extern void snd_run_callbacks(void);
extern void hero_l00_update_frame_2(void);
extern void part_update_all(void);
extern void part_l00_update_effect_slots(void);
extern void cam_update_state(void);
extern s32 UpdateSoundVoices(void);
extern void UpdateLightingEnvironment(void);
extern void gfx_compute_light_direction(void);
extern int hud_update_record_timers(int);
extern void level_run_callbacks(void);
extern void subtitle_update_state_machine(void);
extern void game_update_idle_frame_counters(void);


/* ==== (gc-weapon-wheel) The file's head ends here. ==== */

/* (gc-weapon-wheel) g_hero_state is this file's own view of the hero (GC_SHIM_UNIT_GATE: gc/shim/common.h): the
   Hero struct above has a pointer, which makes it longer on a 64-bit host than on the PS2, so it
   cannot lie over the translation layer's copy of the hero (gc_shim_hero) at Going Commando's
   offsets. It is filled with the one field the kept lines read, the hero's mode byte (+0x22B4). */
char g_hero_state[sizeof(Hero)]; /* (gc-weapon-wheel) */

static void gc_shim_view_hero(void) /* (gc-weapon-wheel) */
{ /* (gc-weapon-wheel) */
    ((Hero *)g_hero_state)->f22B4 = gc_shim_hero[0x22B4]; /* (gc-weapon-wheel) */
} /* (gc-weapon-wheel) */

/* ==== (gc-weapon-wheel) Below, level_update_game_frame_input cut to the weapon quick select's paused open; lines
   not marked are the source's. ==== */
/* Runs one frame of the game: checks button presses for death or pause menus and then runs the normal update steps. */
int level_update_game_frame_input(void) /* (gc-weapon-wheel) returns 1 when it took the tick (Going Commando returns before its world update; here the caller skips the game's) */
{
    int blocked;
    int special;

    /* (gc-weapon-wheel) Lines 87-92 (the death, save and debug slots before the pause) cut: Ratchet & Clank's
       own (handle_pause_input) run after, when this returns 0. */
    gc_shim_view_hero(); /* (gc-weapon-wheel) */

    /* (gc-weapon-wheel) In place of lines 94-105 (`blocked` from Going Commando's hero states 0x31 and its
       vehicles' moby classes, and its level checks): `blocked` (Going
       Commando's name for it, though nonzero lets the quick select open) from Ratchet & Clank's own
       conditions for opening it with Triangle (hero_update_transitions,
       hero_item_input_and_hud.c:110-116: the slot not locked, no timed HUD, the item locks, on foot
       or in mode 3). */
    special = 0; /* (gc-weapon-wheel) */
    blocked = gc_shim_selector_open_allowed(); /* (gc-weapon-wheel) */
    level_update_countdown_state_machine();

    if (PadPressedWithin(16, gc_shim_gate_window(g_pad_press_window), 0) && g_pad_hold_frame_count >= 8) { /* (gc-weapon-wheel) the window in the game's ticks, as Going Commando's PAL build (L00_00371E28-tick-g8.c:119), and long enough for the mod's longer "Hold before pausing" */
        if (blocked && !g_game_l00_mode && !g_input_block_flag && g_language_settings[3]) { /* (gc-weapon-wheel) lines 109-111 with the hero's part (Going Commando's states 0x6F, 0x31, 0x1C, its hero mode 2, its transition value, its mode byte < 2) Ratchet & Clank's, in `blocked` above */
            hud_request_open();
            if (hud_is_open_done()) {
                if (((Hero *)g_hero_state)->f22B4 != 1)
                    hero_update_button_hold_timers();
                if (!request_begin_slot(4, 1, 7, 0, 0))
                    return 1; /* (gc-weapon-wheel) */
            }
        }
    }
    /* (gc-weapon-wheel) Lines 121-199 (the pause and map menus, then the world update) cut: the game's own
       handle_pause_input runs them when this returns 0. */
    return 0; /* (gc-weapon-wheel) */
} /* (gc-weapon-wheel) */
