/* SPDX-License-Identifier: GPL-3.0-only
 *
 * COPIED from Going Commando's decompilation: OpenRAC/rac2-decomp
 * (https://github.com/OpenRAC/rac2-decomp, GNU GPL version 3) at commit
 * 4ea93ceaf43aec055e19211d36512e847860bac2, file src/game/00290670.c,
 * lines 1-13 and 41-54. Code of that project from before its relicensing (d46a3539, 2026-10-11) is also
 * under its MIT notice: THIRD_PARTY_NOTICES.md.
 *
 * hud_request_letterbox_bars (USA v1.01 func_00290670), hud_clear_flag_a and hud_clear_flag_b
 * (func_00290720, func_00290728). In a level the two flags are the words of the quick select's
 * HUD-open request: _a the double tap's swap flag (level_get_flag_d's word), _b the request's
 * state (hud_request_open's): the same code at L00 0x002FC020 and 0x002FC028, and in USA v2.00 the
 * same $gp words (gp-0x5C84, gp-0x5C80) as level_set_flag_d_true and hud_request_open.
 *
 * The copied lines are unchanged. The lines marked (gc-weapon-wheel) are this mod's: the bars' update cut. Every name the copied lines use that is not
 * defined in src/gc/ is the translation layer's (gc/shim/common.h, gc/rac1_shim.cpp): Going
 * Commando's globals and functions in Ratchet & Clank's terms.
 */
#include "common.h"
#include "structs.h"

   /* current buffer */
extern short r1_D_0015EDC4;
extern void hud_request_letterbox_bars(void);
extern int g_letterbox_grow_flag __attribute__((sda));

/* Asks for the screen bars at the top to grow. */
void hud_request_letterbox_bars(void) {
    g_letterbox_grow_flag = 1;
}

/* (gc-weapon-wheel) Lines 14-41 (hud_update_letterbox_bars: the black bars mode 7 grows, 4.8 pixels a
   level) cut: the mod leaves Going Commando's bars out on purpose (Ratchet & Clank's letterbox
   changes its viewport, so the world would zoom). hud_request_letterbox_bars above still sets
   its flag, which nothing reads. */

extern int g_hud_flag_a __attribute__((sda));

/* Clears a HUD flag. */
void hud_clear_flag_a(void) {
    g_hud_flag_a = 0;
}

extern int g_hud_flag_b __attribute__((sda));

/* Clears a second HUD flag. */
void hud_clear_flag_b(void) {
    g_hud_flag_b = 0;
}
