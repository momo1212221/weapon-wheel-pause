/* SPDX-License-Identifier: GPL-3.0-only
 *
 * COPIED from Going Commando's decompilation: OpenRAC/rac2-decomp
 * (https://github.com/OpenRAC/rac2-decomp, GNU GPL version 3) at commit
 * 4ea93ceaf43aec055e19211d36512e847860bac2, file src/game/00285840.c,
 * lines 1-44. Code of that project from before its relicensing (d46a3539, 2026-10-11) is also
 * under its MIT notice: THIRD_PARTY_NOTICES.md.
 *
 * menu_get_saved_state, menu_get_second_value and menu_end (USA v1.01 func_00285840, func_00285850,
 * func_00285860): the modal state's mode and whether the world is drawn under it, and the end of a
 * modal mode (the sounds and the music back on, the pad's history cleared).
 *
 * The copied lines are unchanged. None is changed. Every name the copied lines use that is not
 * defined in src/gc/ is the translation layer's (gc/shim/common.h, gc/rac1_shim.cpp): Going
 * Commando's globals and functions in Ratchet & Clank's terms.
 */
#include "include_asm.h"
#include "names.h"

/* Owner-reviewed Qwen backlog: observed operations, not recovered original object boundaries. */

/* func_00285840: owner ABI/declaration review retained. */
/* extern declaration for global variable referenced in func_00285840 */
extern unsigned int g_menu_saved_state;

/* func_00285850: owner ABI/declaration review retained. */
extern unsigned int g_menu_second_value;

/* func_002858B0: owner ABI/declaration review retained. */
extern int g_menu_setting;

/* Returns the saved menu state value. */
unsigned int menu_get_saved_state(void)
{
    return g_menu_saved_state;
}

/* Returns the second saved menu value. */
unsigned int menu_get_second_value(void)
{
    return g_menu_second_value;
}

extern int request_is_current_slot(int);
extern void snd_send_cmd23(int);
extern void music_Unpause(void);
extern int snd_Pump(void);
extern void pad_clear_port_state(void);

/* Ends the menu: undoes the pause sounds when needed and clears the saved menu state. */
void menu_end(void)
{
    if (request_is_current_slot(3) == 0) {
        snd_send_cmd23(0x5D);
        music_Unpause();
        snd_Pump();
    }
    pad_clear_port_state();
    g_menu_saved_state = 0;
}
