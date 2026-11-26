/* SPDX-License-Identifier: GPL-3.0-only
 *
 * COPIED from Going Commando's decompilation: OpenRAC/rac2-decomp
 * (https://github.com/OpenRAC/rac2-decomp, GNU GPL version 3) at commit
 * 4ea93ceaf43aec055e19211d36512e847860bac2, file src/levels/shared/L00_002FC058-g8.c,
 * lines 1-41 (all). Code of that project from before its relicensing (d46a3539, 2026-10-11) is also
 * under its MIT notice: THIRD_PARTY_NOTICES.md.
 *
 * level_update_countdown_state_machine (USA v1.01 func_L00_002FC058): the quick select's
 * HUD-open request, run each play tick by the gate: asked (1), it counts down a few frames (2) and
 * is done (3) if Triangle is still held then, else cancelled (0); a double tap's swap (flag d)
 * cancels it too. So a tap does not pause, a hold does.
 *
 * The copied lines are unchanged. None is changed. Every name the copied lines use that is not
 * defined in src/gc/ is the translation layer's (gc/shim/common.h, gc/rac1_shim.cpp): Going
 * Commando's globals and functions in Ratchet & Clank's terms.
 */
#include "names.h"
/* func_L00_002FC058 (184 bytes, 27 levels): a small state machine. State 1 arms the
   counter from the saved word; state 2 counts it down, runs the two hooks when
   func_L00_002FC018 says so, and otherwise advances or resets on the 0x10 pad flag.
   The empty case 0 gives retail's slti dispatch. */
#include "common.h"

extern int ref_L00_002FC058_00 __attribute__((sda));
extern int ref_L00_002FC058_01 __attribute__((sda));
extern int ref_L00_002FC058_02 __attribute__((sda));
extern char g_pad_pressed_flags[];

extern int level_get_flag_d(void);
extern void level_clear_flag_e(void);
extern void level_clear_flag_f(void);

void level_update_countdown_state_machine(void)
{
    switch (ref_L00_002FC058_00) {
    case 0:
        break;
    case 1:
        ref_L00_002FC058_00 = 2;
        ref_L00_002FC058_02 = ref_L00_002FC058_01;
        break;
    case 2:
        ref_L00_002FC058_02 = ref_L00_002FC058_02 - 1;
        if (level_get_flag_d() != 0) {
            level_clear_flag_e();
            level_clear_flag_f();
        } else if (ref_L00_002FC058_02 == 0) {
            if (*(int *)g_pad_pressed_flags & 0x10)
                ref_L00_002FC058_00++;
            else
                ref_L00_002FC058_00 = 0;
        } else if ((*(int *)g_pad_pressed_flags & 0x10) == 0) {
            ref_L00_002FC058_00 = 0;
        }
        break;
    }
}
