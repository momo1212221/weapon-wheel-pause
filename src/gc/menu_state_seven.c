/* SPDX-License-Identifier: GPL-3.0-only
 *
 * COPIED from Going Commando's decompilation: OpenRAC/rac2-decomp
 * (https://github.com/OpenRAC/rac2-decomp, GNU GPL version 3) at commit
 * 4ea93ceaf43aec055e19211d36512e847860bac2, file src/game/00287E18.c,
 * lines 1-35 (all). Code of that project from before its relicensing (d46a3539, 2026-10-11) is also
 * under its MIT notice: THIRD_PARTY_NOTICES.md.
 *
 * menu_advance_state_in_mode_seven and menu_is_state_seven (USA v1.01 func_00287E18,
 * func_00287E48): mode 7's phase 1 to 2, and whether mode 7 is on.
 *
 * The copied lines are unchanged. None is changed. Every name the copied lines use that is not
 * defined in src/gc/ is the translation layer's (gc/shim/common.h, gc/rac1_shim.cpp): Going
 * Commando's globals and functions in Ratchet & Clank's terms.
 */
#include "include_asm.h"
#include "names.h"

typedef unsigned char u8;

typedef int s32;

typedef struct {
    u8 prefix[0x40];
    s32 mode;
    u8 between[0x14];
    s32 state;
} StateTransitionView;

extern StateTransitionView g_menu_transition_state;

/* Owner-reviewed Qwen backlog: observed operations, not recovered original object boundaries. */

/* func_00285840: owner ABI/declaration review retained. */
/* extern declaration for global variable referenced in func_00285840 */
extern unsigned int g_menu_saved_state;

/* Change resident state word from1 to2 only when the mode word equals7 */
/* Moves the menu state from 1 to 2, but only while the menu mode is 7. */
void menu_advance_state_in_mode_seven(void) {
    if (g_menu_transition_state.mode == 7 && g_menu_transition_state.state == 1) {
        g_menu_transition_state.state = 2;
    }
}

/* Returns true while the saved menu state is 7. */
int menu_is_state_seven(void)
{
    return g_menu_saved_state == 7;
}
