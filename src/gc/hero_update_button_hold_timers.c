/* SPDX-License-Identifier: GPL-3.0-only
 *
 * COPIED from Going Commando's decompilation: OpenRAC/rac2-decomp
 * (https://github.com/OpenRAC/rac2-decomp, GNU GPL version 3) at commit
 * 4ea93ceaf43aec055e19211d36512e847860bac2, file src/levels/shared/L00_002CB218.c,
 * lines 1-40 (all). Code of that project from before its relicensing (d46a3539, 2026-10-11) is also
 * under its MIT notice: THIRD_PARTY_NOTICES.md.
 *
 * hero_update_button_hold_timers (USA v1.01 func_L00_002CB218): Triangle's double tap, the quick
 * swap. Built with GC_SHIM_HANDLES (CMakeLists.txt): it passes an address as an s32.
 *
 * The copied lines are unchanged. The two lines marked (gc-weapon-wheel) are this mod's: the 20 frames in the game's ticks. Every name the copied lines use that is not
 * defined in src/gc/ is the translation layer's (gc/shim/common.h, gc/rac1_shim.cpp): Going
 * Commando's globals and functions in Ratchet & Clank's terms.
 */
#include "include_asm.h"
#include "common.h"

extern char g_hero_button_hold_timers[];
extern char g_pad_state[];
extern int g_input_block_flag __attribute__((section(".sdata")));
extern s32 FastDecTimer(s32);
extern void level_set_flag_d_true(void);

/* Counts down the hero's button-hold timers and starts the special action when the button is pressed again in time. */
void hero_update_button_hold_timers(void)
{
    char *hero;
    char *pad;
    u64 timer;

    FastDecTimer(g_hero_button_hold_timers);
    FastDecTimer(g_hero_button_hold_timers + 2);
    hero = g_hero_button_hold_timers - 0x24F8;
    if (0x37 == *(int *)(hero + 0x2294))
        return;
    if (*(u8 *)(hero + 0x22BC) != 0)
        return;
    if (g_input_block_flag != 0)
        return;
    pad = g_pad_state;
    if (*(int *)(pad + 0x1A4) & 0x10)
        *(short *)(hero + 0x24F8) = gc_shim_frames(20); /* (gc-weapon-wheel) 20 frames in the game's ticks */
    timer = *(u32 *)(hero + 0x24F8);
    if (timer == 0)
        return;
    if (*(int *)(pad + 0x1B8) & 0x10) {
        if (*(short *)(hero + 0x24FA) == 0) {
            *(short *)(hero + 0x24FA) = gc_shim_frames(20); /* (gc-weapon-wheel) 20 frames in the game's ticks */
            return;
        }
        *(int *)(hero + 0x2538) = 1;
        level_set_flag_d_true();
    }
}
