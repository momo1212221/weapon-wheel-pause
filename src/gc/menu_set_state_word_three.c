/* SPDX-License-Identifier: GPL-3.0-only
 *
 * COPIED from Going Commando's decompilation: OpenRAC/rac2-decomp
 * (https://github.com/OpenRAC/rac2-decomp, GNU GPL version 3) at commit
 * 4ea93ceaf43aec055e19211d36512e847860bac2, file src/game/00287E60.c,
 * lines 1-10. Code of that project from before its relicensing (d46a3539, 2026-10-11) is also
 * under its MIT notice: THIRD_PARTY_NOTICES.md.
 *
 * menu_set_state_word_three (USA v1.01 func_00287E60): mode 7's phase-1 timer set to 3 frames, by
 * mode 7's phase 0 and by the quick select's update each frame it is active, so the pause lasts
 * while the quick select is. ref_L00_002F3458_00 is that timer (g_modal_timer): USA v2.00's level
 * programs write 3 to it here (level 0: 0x002F4238, `sw $v0, -0x6240($gp)`) and UpdateModalState's
 * phase 1 counts it down (0x002F3BAC-0x002F3BC4).
 *
 * The copied lines are unchanged. None is changed. Every name the copied lines use that is not
 * defined in src/gc/ is the translation layer's (gc/shim/common.h, gc/rac1_shim.cpp): Going
 * Commando's globals and functions in Ratchet & Clank's terms.
 */
#include "include_asm.h"
#include "names.h"

extern int ref_L00_002F3458_00;

/* Sets a global state word to 3. */
void menu_set_state_word_three(void)
{
    ref_L00_002F3458_00 = 3;
}
