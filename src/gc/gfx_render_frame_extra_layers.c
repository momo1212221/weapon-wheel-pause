/* SPDX-License-Identifier: GPL-3.0-only
 *
 * COPIED from Going Commando's decompilation: OpenRAC/rac2-decomp
 * (https://github.com/OpenRAC/rac2-decomp, GNU GPL version 3) at commit
 * 4ea93ceaf43aec055e19211d36512e847860bac2, file src/game/0027B948.c,
 * lines 1-3 and 124-142. Code of that project from before its relicensing (d46a3539, 2026-10-11) is also
 * under its MIT notice: THIRD_PARTY_NOTICES.md.
 *
 * gfx_render_frame_extra_layers (USA v1.01 func_0027C4A8): game mode 4's picture: the world (and
 * its HUD) drawn again when the mode was entered from play, then the modal mode's message panel.
 *
 * The copied lines are unchanged. The line marked (gc-weapon-wheel) is this mod's: Ratchet & Clank's loading guard. Every name the copied lines use that is not
 * defined in src/gc/ is the translation layer's (gc/shim/common.h, gc/rac1_shim.cpp): Going
 * Commando's globals and functions in Ratchet & Clank's terms.
 */
#include "ps2_hw.h"
#include "names.h"
#include "include_asm.h"

extern unsigned int menu_get_second_value(void);
extern void gfx_append_draw_list_buffer_tag(int);
extern int g_render_layer_flags __attribute__((sda));
extern void RenderFrame(void);
extern void menu_draw_pause_panels(int);
extern void menu_draw_message_panel(void);

/* Draws a frame with extra layers switched on when the screen mode asks for it, then finishes the frame. */
void gfx_render_frame_extra_layers(void)
{
    if (gc_shim_skip_frame()) return; /* (gc-weapon-wheel) Ratchet & Clank's guard (draw_dialog_background_and_text): nothing while a level loads */
    if (menu_get_second_value() == 1) {
        gfx_append_draw_list_buffer_tag(0);
        g_render_layer_flags = 0x100FF;
        RenderFrame();
    } else if (menu_get_second_value() == 0) {
        menu_draw_pause_panels(1);
    }
    menu_draw_message_panel();
}
