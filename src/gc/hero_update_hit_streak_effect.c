/* SPDX-License-Identifier: GPL-3.0-only
 *
 * COPIED from Going Commando's decompilation: OpenRAC/rac2-decomp
 * (https://github.com/OpenRAC/rac2-decomp, GNU GPL version 3) at commit
 * 4ea93ceaf43aec055e19211d36512e847860bac2, file src/levels/shared/L00_003266D8-g8.c,
 * lines 1-27 and 42. Code of that project from before its relicensing (d46a3539, 2026-10-11) is also
 * under its MIT notice: THIRD_PARTY_NOTICES.md.
 *
 * hero_update_hit_streak_effect (USA v1.01 func_L00_003266D8), cut to its first block: the press
 * window (g_pad_press_window, +1 a frame up to 10; ref_L00_003266D8_00 is the same word).
 *
 * The copied lines are unchanged. The line marked (gc-weapon-wheel) is this mod's: the rest cut. Every name the copied lines use that is not
 * defined in src/gc/ is the translation layer's (gc/shim/common.h, gc/rac1_shim.cpp): Going
 * Commando's globals and functions in Ratchet & Clank's terms.
 */
#include "names.h"
typedef int s32;

extern s32 g_input_block_flag;
extern s32 g_pad_press_window;
extern s32 g_hit_streak_state;
extern s32 ref_L00_003266D8_00 __attribute__((sda));
extern s32 ref_L00_003266D8_01 __attribute__((sda));
extern s32 g_hit_streak_enabled __attribute__((sda));
extern s32 g_hit_streak_hud_slot __attribute__((sda));
extern unsigned char g_hit_streak_hud_data_a[];
extern unsigned char g_hit_streak_hud_data_b[];
extern unsigned char g_hit_streak_hud_data_c[];
extern s32 hud_set_slot_contents(s32, s32, void *, void *, void *, s32, s32);
extern s32 level_raise_record_peak(s32, s32);

/* Counts down the hit streak timers each frame and shows or hides the streak effect while the streak is active. */
void hero_update_hit_streak_effect(void) {
    if (g_input_block_flag == 0) {
        s32 count = g_pad_press_window + 1;

        ref_L00_003266D8_00 = count;
        if (count > 10)
            ref_L00_003266D8_00 = 10;
    } else {
        g_pad_press_window = 0;
    }
    /* (gc-weapon-wheel) Lines 28-41 (the hit streak's timer and HUD, which Ratchet & Clank does not have) cut. */
}
