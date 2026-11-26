/* SPDX-License-Identifier: GPL-3.0-only
 *
 * COPIED from Going Commando's decompilation: OpenRAC/rac2-decomp
 * (https://github.com/OpenRAC/rac2-decomp, GNU GPL version 3) at commit
 * 4ea93ceaf43aec055e19211d36512e847860bac2, file src/game/0028BDB0.c,
 * lines 1-19. Code of that project from before its relicensing (d46a3539, 2026-10-11) is also
 * under its MIT notice: THIRD_PARTY_NOTICES.md.
 *
 * menu_init_panel_layout (USA v1.01 func_0028BDB0): the weapon quick select's init callback.
 *
 * The copied lines are unchanged. The line marked (gc-weapon-wheel) is this mod's: the arming countdown in the game's ticks. Every name the copied lines use that is not
 * defined in src/gc/ is the translation layer's (gc/shim/common.h, gc/rac1_shim.cpp): Going
 * Commando's globals and functions in Ratchet & Clank's terms.
 */
#include "include_asm.h"
#include "names.h"

typedef unsigned char u8;

typedef unsigned short u16;

typedef int s32;

extern void weapon_menu_init_slots(void);

/* authored C for independently verified level-native body */
/* Sets up a pop-up menu panel with its size and starting values. */
void menu_init_panel_layout(u8 *panel) {
 weapon_menu_init_slots();
 *(s32 *)(panel+0x58)=210;*(s32 *)(panel+0x5c)=200;
 *(s32 *)(panel+0x74)=-2;*(s32 *)(panel+0x78)=gc_shim_frames(30); /* (gc-weapon-wheel) 30 frames in the game's ticks (Ratchet & Clank: scale_ticks(0x1E)) */
 *(u16 *)(panel+0x48)=0;*(u16 *)(panel+0x4a)=0;*(s32 *)(panel+0x70)=0;
}
