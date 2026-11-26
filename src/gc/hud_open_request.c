/* SPDX-License-Identifier: GPL-3.0-only
 *
 * COPIED from Going Commando's decompilation: OpenRAC/rac2-decomp
 * (https://github.com/OpenRAC/rac2-decomp, GNU GPL version 3) at commit
 * 4ea93ceaf43aec055e19211d36512e847860bac2, file src/game/00290730.c,
 * lines 1-19. Code of that project from before its relicensing (d46a3539, 2026-10-11) is also
 * under its MIT notice: THIRD_PARTY_NOTICES.md.
 *
 * hud_request_open and hud_is_open_done (USA v1.01 func_00290730, func_00290748): the quick
 * select's HUD-open request (0 none, 1 asked, 2 counting, 3 done), which
 * level_update_countdown_state_machine runs. ref_L00_002FC030_00 and ref_L00_002FC048_00 are
 * that machine's word (ref_L00_002FC058_00: gp-0x5C80 in USA v2.00's level programs).
 *
 * The copied lines are unchanged. None is changed. Every name the copied lines use that is not
 * defined in src/gc/ is the translation layer's (gc/shim/common.h, gc/rac1_shim.cpp): Going
 * Commando's globals and functions in Ratchet & Clank's terms.
 */
#include "include_asm.h"
#include "names.h"

extern int ref_L00_002FC030_00 __attribute__((sda));

extern int ref_L00_002FC048_00 __attribute__((sda));

/* Starts a request if none is running yet. */
void hud_request_open(void)
{
    if (ref_L00_002FC030_00 == 0)
        ref_L00_002FC030_00 = 1;
}

/* Tells whether the request has reached its finished state. */
int hud_is_open_done(void)
{
    return ref_L00_002FC048_00 == 3;
}
