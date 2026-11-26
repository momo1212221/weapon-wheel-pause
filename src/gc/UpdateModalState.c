/* SPDX-License-Identifier: GPL-3.0-only
 *
 * COPIED from Going Commando's decompilation: OpenRAC/rac2-decomp
 * (https://github.com/OpenRAC/rac2-decomp, GNU GPL version 3) at commit
 * 4ea93ceaf43aec055e19211d36512e847860bac2, file src/game/00286738-g8.c,
 * lines 1-177 (the head and the prologue), 593-661 (case 7) and 765-767. Code of that project from before its relicensing (d46a3539, 2026-10-11) is also
 * under its MIT notice: THIRD_PARTY_NOTICES.md.
 *
 * UpdateModalState (USA v1.01 func_00286738): Going Commando's game mode 4 tick, cut to its modal
 * mode 7, the paused weapon quick select. Here it runs in Ratchet & Clank's game mode 4 (the freeze
 * mode, update_mode_freeze), for the mod's freeze kind (rac1.freeze.update).
 *
 * The copied lines are unchanged. The lines marked (gc-weapon-wheel) are this mod's: the other modes cut. Every name the copied lines use that is not
 * defined in src/gc/ is the translation layer's (gc/shim/common.h, gc/rac1_shim.cpp): Going
 * Commando's globals and functions in Ratchet & Clank's terms.
 */
/* func_00286738 needs its own unit: -O2 -G8 -ffunction-sections -frac2-zero-ti-por
 * (config/cflags.txt). D_001B17D0's sda declaration gives retail's repeated
 * lui/addiu; D_0018C0D4, D_001F29A0 and D_001F2930 need absolute addressing;
 * D_001B1680 uses an incomplete sda view for its absolute store macro.
 * func_002AA548 returns float; func_0028B3D0 and func_002E4AC8 return int.
 * ref_00286738_00 is 0x1A8A44 in the boot and 0x1A8D94 in every level, where
 * func_L00_003A0C00 uses the name D_001A8A44 for a different word at 0x1A8A44.
 */

#include "common.h"

/* Modal state observed by the menu and camera transition handlers. */
typedef int Quadword __attribute__((mode(TI)));
typedef struct {
    float cameraPosition[4];
    float cameraAngles[4];
    float returnPosition[4];
    float returnAngles[4];
    int mode;
    int countdown;
    char reserved48[12];
    int selection;
    int phase;
    int elapsed;
    int inputDelay;
    int reserved64;
    int pending;
    float fraction;
    float angle;
    char reserved74[8];
    short timer;
    short cameraActive;
    int savedValue;
    short result;
    short reserved86;
    int enabled;
    unsigned int acceptMask;
    unsigned int cancelMask;
    int gameState;
} ModalState;

typedef struct {
    char reserved00[0x100];
    float roll;
    char reserved104[0x44];
    float horizontal;
    float vertical;
    char reserved150[0x54];
    unsigned int buttons;
    char reserved1A8[0x18];
    unsigned int held;
    unsigned int pressed;
} ModalInput;

typedef struct CameraObject CameraObject;
struct CameraObject {
    char reserved00[0x64];
    void (*update)(CameraObject *);
    char reserved68[0x42];
    short kind;
    char reservedAC[0x54];
};

extern ModalState g_modal_state;
extern ModalInput g_pad_state;
extern int g_vis_fallback_mode;
extern int g_save_menu_screen;
extern int g_save_menu_flags;
extern int g_menu_panel_active;
extern int g_menu_panel_flag;
extern int g_message_panel_flags NOT_SDA;
typedef struct {
    char reserved00[0x18];
    int selection;
    char reserved1C[0xE4];
    int savedSelection;
} ModalSelection;
extern ModalSelection g_pause_menu_state;
extern int *g_save_packet_buffer NOT_SDA;
extern unsigned short g_subtitle_history[];
extern unsigned char g_render_flag_byte;
typedef struct {
    char reserved00[8];
    int status;
    char reserved0C[4];
    int result;
} ModalCardState;
extern ModalCardState g_memcard_state;
extern int g_modal_state_word[];
extern int g_frontend_done_flag __attribute__((sda));
extern int g_small_data_value[] __attribute__((sda));
extern int g_modal_distance __attribute__((sda));
extern float g_modal_speed_scale __attribute__((sda));
typedef struct {
    float position[4];
    float angles[4];
    char reserved20[0x210];
    float matrix[16];
} ModalCamera;
extern ModalCamera g_camera_eye_state;
extern float g_camera_vector_scratch[4] __attribute__((sda));
extern unsigned char g_cheat_flags;
extern int g_current_map_index;
extern CameraObject *g_moby_pool_start;
extern CameraObject *g_moby_pool_end;
extern unsigned char g_screen_mode_flag NOT_SDA;
extern int g_modal_counter __attribute__((sda));
extern int ref_00286738_00 __attribute__((sda));
extern int g_modal_clear_flag __attribute__((sda));
extern int g_modal_timer __attribute__((sda));
extern int g_widescreen_flag;

extern int UpdateSoundVoices(void);
extern int request_begin_slot(int, int, int, int, unsigned char *);
extern void FadeToBlack(int);
extern int flag_get_word(void);
extern void flag_clear_word(void);
extern void menu_clear_counter(void);
extern int menu_is_known_screen_active(void);
extern int menu_get_saved_value(void);
extern void save_load_packet(int *);
extern void menu_clear_confirm_pending(void);
extern int request_end_slot(int, int);
extern void save_load_game_data_reset_menu(void);
extern void load_data_and_setup_callback(void);
extern void level_restart_keep_progress(int);
extern void FastVecSub(void *, const void *, const void *);
extern float FastVecLength(void *);
extern float math_step_value_toward_target(float *, float *, float, float, float, float);
extern void vec_scale_to_length(void *, const void *, float);
extern void FastVecAdd(void *, const void *, const void *);
extern void cam_update_follow_rotation(char *, int);
extern void matrix_mul_vec3(void *, const void *, const void *);
extern void vec_clamp_length(float *, float);
extern void FastVecScale(void *, const void *, float);
extern void cam_offset_direction(char *);
extern void cam_apply_roll(float *);
extern void matrix_from_basis_rows(void *, const void *);
extern void mat_get_euler_angles(float *, float *);
extern void lerp_vector(void *, const void *, const void *, float);
extern void mat_build_rotation_from_angles(void *, float *);
extern void vec_fast_normalize3(void *, const void *, const void *, float);
extern void quaternion_to_rotation_basis(const void *, void *);
extern void stub_empty_20(void);
extern void hud_request_letterbox_bars(void);
extern void menu_set_state_word_three(void);
extern int hud_set_slot_contents(int, int, void (*)(unsigned char *), void (*)(), void (*)(), int, int);
extern void menu_init_panel_layout(unsigned char *);
extern void weapon_wheel_update_select();
extern int weapon_wheel_draw();
extern void weapon_wheel_init(unsigned char *);
extern void UpdateRadialMenuSelection();
extern int DrawRadialSelectorPanel();
extern void func_0029CE38(void);
extern void level_forward_value_b(int);
extern void level_forward_value(int);
extern void hud_clear_flag_b(void);
extern void hud_clear_flag_a(void);
extern void request_set_setting_b(int);
extern int snd_play_from_global_bank(int, int, void *);
extern void gfx_apply_screen_aspect_mode(void);

/* Run the active modal dialog, camera return, or display-confirmation phase. */
void UpdateModalState(void)
{
    float delta[4], target[4], transform[16];
    float distance;
    ModalState *loadDialog, *saveDialog, *phaseDialog;
    ModalInput *saveInput;

    UpdateSoundVoices();
    g_vis_fallback_mode = g_modal_state.gameState;
    if (g_modal_state.countdown)
        --g_modal_state.countdown;
    if (g_modal_state.timer)
        --g_modal_state.timer;
    switch (g_modal_state.mode) {
    /* (gc-weapon-wheel) Lines 178-592 (modes 2 to 6: Going Commando's save, load and camera dialogs) cut. */
    case 7:
        switch (g_modal_state.phase) {
        case 0:
            g_modal_state.cameraActive = 1;
            g_modal_state.phase = 1;
            if (g_screen_mode_flag != 1)
                stub_empty_20();
            g_modal_clear_flag = 0;
            g_modal_counter = ref_00286738_00;
            hud_request_letterbox_bars();
            menu_set_state_word_three();
            break;
        case 1:
            if (g_screen_mode_flag != 1) {
                stub_empty_20();
                hud_set_slot_contents(0x23, 0, menu_init_panel_layout, weapon_wheel_update_select, weapon_wheel_draw, 0, 0);
            } else {
                hud_set_slot_contents(0x23, 0, weapon_wheel_init, UpdateRadialMenuSelection, DrawRadialSelectorPanel, 0, 0);
            }
            func_0029CE38();
            level_forward_value_b(360);
            level_forward_value(180);
            hud_request_letterbox_bars();
            {
                int remaining = g_modal_counter;
                if (remaining > 0) {
                    g_modal_counter = remaining - 1;
                    remaining = g_modal_counter;
                }
                g_modal_counter = remaining;
            }
            --g_modal_timer;
            if (g_modal_timer < 0)
                g_modal_timer = 0;
            if (!g_modal_timer)
                ++g_modal_state.phase;
            break;
        case 2:
            if (g_screen_mode_flag != 1) {
                stub_empty_20();
                hud_set_slot_contents(0x23, 0, menu_init_panel_layout, weapon_wheel_update_select, weapon_wheel_draw, 0, 0);
            } else {
                hud_set_slot_contents(0x23, 0, weapon_wheel_init, UpdateRadialMenuSelection, DrawRadialSelectorPanel, 0, 0);
            }
            func_0029CE38();
            level_forward_value_b(360);
            level_forward_value(180);
            hud_request_letterbox_bars();
            {
                int remaining = g_modal_counter;
                if (remaining > 0) {
                    g_modal_counter = remaining - 1;
                    remaining = g_modal_counter;
                }
                g_modal_counter = remaining;
            }
            if (g_modal_counter <= 0) {
                g_modal_counter = 0;
                ++g_modal_state.phase;
            }
            break;
        case 3:
            if (g_screen_mode_flag != 1)
                stub_empty_20();
            hud_clear_flag_b();
            hud_clear_flag_a();
            { request_end_slot(0, 0); break; }
        }
        break;
    /* (gc-weapon-wheel) Lines 662-764 (modes 8 to 11: its other dialogs) cut. */
    }

}
