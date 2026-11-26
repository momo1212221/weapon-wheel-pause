/* SPDX-License-Identifier: GPL-3.0-only
 *
 * COPIED from Going Commando's decompilation: OpenRAC/rac2-decomp
 * (https://github.com/OpenRAC/rac2-decomp, GNU GPL version 3) at commit
 * 4ea93ceaf43aec055e19211d36512e847860bac2, file src/game/0029BCC8.c,
 * lines 1-2, 14 and 520-532. Code of that project from before its relicensing (d46a3539, 2026-10-11) is also
 * under its MIT notice: THIRD_PARTY_NOTICES.md.
 *
 * func_0029CE38: the HUD records' update (hud_update_record_timers(1)) and the level's callbacks,
 * which mode 7 runs each frame while the world is held.
 *
 * The copied lines are unchanged. None is changed. Every name the copied lines use that is not
 * defined in src/gc/ is the translation layer's (gc/shim/common.h, gc/rac1_shim.cpp): Going
 * Commando's globals and functions in Ratchet & Clank's terms.
 */
#include "include_asm.h"
#include "names.h"

extern char *g_resident_data_base;

/* If the menu system exists and the game is not paused, turns something on and runs a follow-up setup step. */
void func_0029CE38(void)
{
    extern int g_notice_owner;
    extern int hud_update_record_timers(int);
    extern void level_run_callbacks(void);
    if (g_resident_data_base != 0) {
        if (g_notice_owner == 0) {
            hud_update_record_timers(1);
            level_run_callbacks();
        }
    }
}
