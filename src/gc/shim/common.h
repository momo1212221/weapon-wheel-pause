/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * The translation layer's C side: what the copied Going Commando files (src/gc/*.c) include as
 * "common.h" (and "names.h", "include_asm.h", "structs.h", "ps2_hw.h") in place of rac2-decomp's.
 * It gives them rac2-decomp's types and markers, and renames every global and function they use
 * that is not itself copied to the translation layer's (gc/rac1_shim.cpp), which puts Going
 * Commando's quick select, modal state and functions in Ratchet & Clank's terms. Nothing of
 * rac2-decomp's own headers is copied.
 *
 * Three kinds of global:
 *  - In game memory, the same word in both games: `NAME` becomes `(*gc_shim_NAME)`, so the copied
 *    declaration (`extern int NAME;`, `extern char NAME[];`, `extern Struct NAME;`) declares a host
 *    pointer to what it declared, which the layer points at Ratchet & Clank's word for the level
 *    loaded. The pad (PadState: the same offsets for every field the copied code reads), the
 *    quick select's page, slot count, defaults and close bookkeeping, the ammo, the game mode.
 *  - A copy, where Ratchet & Clank keeps the same thing at other offsets: the hero (gc_shim_hero, at
 *    Going Commando's offsets), the usage statistics, the weapon table (one Going Commando record
 *    for each Ratchet & Clank item, from item_defs and item_price_table). The layer fills it before
 *    the copied code runs and writes back what it changed.
 *  - The mod's own, where Ratchet & Clank has none: the modal state of game mode 4, the HUD-open
 *    request,
 *    the press window, the HUD flags.
 * gc/rac1_shim.hpp says which is which.
 */
#ifndef GC_WEAPON_WHEEL_SHIM_COMMON_H
#define GC_WEAPON_WHEEL_SHIM_COMMON_H

/* rac2-decomp's types (include/common.h). */
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;
typedef float f32;
typedef double f64;

/* Placement markers for the PS2's small-data area: nothing on the host. */
#define MACRO_ADDR
#define NOT_SDA
#define sda unused
#define section(...) unused

/* --- Globals in game memory: a host pointer to Ratchet & Clank's word --- */

/* The pad (g_controller_state): Going Commando's PadState is Ratchet & Clank's at every offset the
   copied code reads (the stick 0x148/0x14C, held 0x1A0 and 0x1C0, pressed 0x1A4 and 0x1C4, raw
   released 0x1B8, mode 0x1CC, stick moved 0x1D8, the history 0x18E/0x190/0x1E0). */
#define g_pad_state (*gc_shim_pad)
/* The quick select's page index (quick_select_page), slot count (quick_select_slot_count), the
   default slots for the d-pad with nothing selected ([2] left, [3] right, [4] up, [5] down:
   quick_select_default_left_slot.. less two words), the stick's angle off the slot
   (quick_select_slot_angle_offset), and what a close leaves (the last selection and state, and the
   frame: quick_select_last_state, quick_select_close_frame). */
#define g_radial_menu_page_index (*gc_shim_page_index)
#define g_radial_slot_count (*gc_shim_slot_count)
#define g_wheel_default_choices (*gc_shim_default_choices)
#define g_wheel_angle_delta (*gc_shim_angle_delta)
#define g_weapon_wheel_selected (*gc_shim_saved_selected)
#define g_weapon_wheel_state (*gc_shim_saved_state)
#define g_weapon_wheel_open_frame (*gc_shim_close_frame)
#define g_frame_counter_3 (*gc_shim_rendered_frame_count)
/* The eight items of the slots (the slot assignment) and page 0's entries (7 words each: icon,
   left, right, up, down at +8..+0x14, item at +0x18). */
#define g_weapon_menu_slots (*gc_shim_slot_items)
#define g_weapon_menu_slots_2 (*gc_shim_page0_entries)
/* Ammo per item (weapon_ammo_counts), the ammo text's format (hud_ammo_format). */
#define g_item_ammo_counts (*gc_shim_ammo)
#define g_str_ammo_format (*gc_shim_ammo_format)
/* The quick select's close timer (hero+0x24F6 in Going Commando: Ratchet & Clank's
   hud_item_selector_close_timer, hero+0x22D2). */
#define g_weapon_wheel_timer (*gc_shim_wheel_timer)
/* The level's number. */
#define g_current_map_index (*gc_shim_level_index)
/* The game mode (Ratchet & Clank's g_game_mode), the level's mode (cinematic_mode) and the frames
   since it changed (cinematic_mode_frames): the gate's `>= 8`. */
#define g_current_game_mode (*gc_shim_game_mode)
#define g_game_l00_mode (*gc_shim_cinematic_mode)
#define g_pad_hold_frame_count (*gc_shim_cinematic_frames)
/* The draw mask the world's render reads (debug_profiler_color). */
#define g_render_layer_flags (*gc_shim_draw_mask)
/* The modal state's mode (+0x40) and whether the world is drawn under it (+0x88). */
#define g_menu_saved_state (*gc_shim_menu_saved_state)
#define g_menu_second_value (*gc_shim_menu_second_value)
/* Going Commando's hero mode byte (hero+0x22B4, `g_screen_mode_flag`): the copy's. */
#define g_screen_mode_flag (*gc_shim_screen_mode_flag)
/* Triangle's double-tap timers (hero+0x24F8, +0x24FA): the copy's. */
#define g_hero_button_hold_timers (*gc_shim_hold_timers)

/* The page table: host pointers to each page's entries, which the layer builds from Ratchet &
   Clank's table of 32-bit game addresses (hud_selector_slot_tables). weapon_menu_init_slots writes
   Ratchet & Clank's word itself (GC_SHIM_UNIT_INIT_SLOTS): the table's address as a game address,
   which `&g_radial_pages_data` gives there as the layer's pointer holds it (never dereferenced). */
#if defined(GC_SHIM_UNIT_INIT_SLOTS)
#define g_radial_menu_pages (*gc_shim_rac1_page_table)
#define g_radial_pages_data (*gc_shim_rac1_pages_data)
#else
#define g_radial_menu_pages gc_shim_menu_pages
#endif

/* --- Copies at Going Commando's offsets --- */

/* The hero (D_00189E20): items[0].item_id at +0x1248, pending_item[0] at +0x22C8, the mode byte
   (vehicle_mode) at +0x22B4, the item lock at +0x22BC, the double-tap timers at +0x24F8. The gate's
   file has its own view (GC_SHIM_UNIT_GATE: see there). */
#if defined(GC_SHIM_UNIT_GATE)
extern unsigned char gc_shim_hero[];
#else
#define g_hero_state gc_shim_hero
#endif
/* The usage statistics' quick select entry, at Going Commando's +0xF0 (Ratchet & Clank's
   usage_stats entry 20, at +0xA0), and the "frame counter" it compares with (Ratchet & Clank's
   scale_ticks(gameplay_event_frame_count) / 600, which its own entry keeps). */
#define g_progress_statistics gc_shim_stats
#define g_frame_counter gc_shim_frame_counter
/* The weapon table: a 0xE0-byte Going Commando record for each Ratchet & Clank item, and the map
   from an item to its record (the item itself). */
#define g_weapon_table gc_shim_weapon_table
#define g_weapon_l00_table gc_shim_weapon_table
#define g_item_slot_to_record gc_shim_item_record

/* --- The mod's own --- */

#define g_modal_state gc_shim_modal_state
#define g_menu_transition_state gc_shim_modal_state
#define g_modal_state_word gc_shim_modal_state_word
#define g_menu_mode gc_shim_menu_mode
#define g_vis_fallback_mode gc_shim_vis_fallback_mode
#define g_modal_counter gc_shim_modal_counter
#define ref_00286738_00 gc_shim_modal_counter_start
#define g_modal_clear_flag gc_shim_modal_clear_flag
/* Mode 7's phase-1 timer: menu_set_state_word_three's word too (USA v2.00: gp-0x6240). */
#define g_modal_timer gc_shim_modal_timer
#define ref_L00_002F3458_00 gc_shim_modal_timer
#define g_letterbox_grow_flag gc_shim_letterbox_grow_flag
/* The quick select's HUD-open request (hud_open_request.c, level_update_countdown_state_machine.c).
   In a level, Going Commando keeps it in two words, the same in every function that names them
   (USA v2.00's level programs: gp-0x5C80 and gp-0x5C84): the request's state, which
   hud_request_open, hud_is_open_done, the state machine and hud_clear_flag_b (level_clear_flag_f)
   use, and the double tap's swap flag, which level_set_flag_d_true, level_get_flag_d and
   hud_clear_flag_a (level_clear_flag_e) use. The countdown's start (ref_L00_002FC058_01, 4 frames in
   Going Commando's data) is set in the game's ticks by the layer. */
#define g_hud_flag_b gc_shim_hud_flag_b
#define ref_L00_002FC030_00 gc_shim_hud_flag_b
#define ref_L00_002FC048_00 gc_shim_hud_flag_b
#define ref_L00_002FC058_00 gc_shim_hud_flag_b
#define g_hud_flag_a gc_shim_hud_flag_a
#define ref_L00_002FC058_01 gc_shim_open_delay_start
#define ref_L00_002FC058_02 gc_shim_open_delay_counter
#define level_clear_flag_e hud_clear_flag_a
#define level_clear_flag_f hud_clear_flag_b
/* The pad's held word the state machine tests (D_00138320, the pad + 0x1A0: Ratchet & Clank's held). */
#define g_pad_pressed_flags (*gc_shim_pad_held)
#define g_pad_press_window gc_shim_press_window
#define ref_L00_003266D8_00 gc_shim_press_window
#define g_input_block_flag gc_shim_input_block
#define g_notice_owner gc_shim_notice_owner
#define g_resident_data_base gc_shim_resident_data_base
#define g_wheel_fade_count gc_shim_wheel_fade_count
#define g_sequence_active_flag gc_shim_sequence_active
#define g_str_wheel_debug gc_shim_str_wheel_debug
/* Going Commando's option bytes: [3] (D_001A7BB3, g_weapon_wheel_enabled) is "Pause while open". */
#define g_language_settings gc_shim_options
#define D_001A7BB0 gc_shim_options
#define g_weapon_wheel_enabled D_001A7BB3

/* --- Functions: the translation layer's (gc/rac1_shim.cpp says what each one does here) --- */

#if defined(GC_SHIM_HANDLES)
/* hero_update_button_hold_timers passes the timers' address as an s32: a handle the layer finds
   again in its copy of the hero. */
#define FastDecTimer gc_shim_h_FastDecTimer
#else
#define FastDecTimer gc_shim_FastDecTimer
#endif
#define DrawRadialSelectorPanel gc_shim_DrawRadialSelectorPanel
#define FastDiffRots gc_shim_FastDiffRots
#define FastTweenColor gc_shim_FastTweenColor
#define FloatToInt gc_shim_FloatToInt
#define GetLocalizedString gc_shim_GetLocalizedString
#define IntToFloat gc_shim_IntToFloat
#define LoadClassIntoFreeCacheSlot gc_shim_LoadClassIntoFreeCacheSlot
#define RenderFrame gc_shim_RenderFrame
#define STUB_printf gc_shim_STUB_printf
#define UpdateRadialMenuSelection gc_shim_UpdateRadialMenuSelection
#define UpdateSoundVoices gc_shim_UpdateSoundVoices
#define fast_subtract_rotations gc_shim_fast_subtract_rotations
#define func_001157AC gc_shim_strlen
#define gfx_append_draw_list_buffer_tag gc_shim_gfx_append_draw_list_buffer_tag
#define hud_close_element_by_key gc_shim_hud_close_element_by_key
#define hud_screen_position gc_shim_hud_screen_position
#define hud_set_slot_contents gc_shim_hud_set_slot_contents
#define hud_update_record_timers gc_shim_hud_update_record_timers
#define level_forward_value gc_shim_level_forward_value
#define level_forward_value_b gc_shim_level_forward_value_b
#define level_get_flag_d gc_shim_level_get_flag_d
#define level_run_callbacks gc_shim_level_run_callbacks
#define level_set_flag_d_true gc_shim_level_set_flag_d_true
#define load_is_class_cached gc_shim_load_is_class_cached
#define load_request_disc_read_break gc_shim_load_request_disc_read_break
#define mat_fast_helper_a gc_shim_mat_fast_helper_a
#define math_fast_float_g gc_shim_math_fast_float_g
#define menu_draw_message_panel gc_shim_menu_draw_message_panel
#define menu_draw_pause_panels gc_shim_menu_draw_pause_panels
#define menu_set_screen_value_restart_six_fades gc_shim_menu_set_screen_value_restart_six_fades
#define mode_get_stack_top gc_shim_mode_get_stack_top
#define music_Pause gc_shim_music_Pause
#define music_Unpause gc_shim_music_Unpause
#define pad_clear_port_state gc_shim_pad_clear_port_state
#define request_begin_slot gc_shim_request_begin_slot
#define request_end_slot gc_shim_request_end_slot
#define request_is_current_slot gc_shim_request_is_current_slot
#define snd_Pump gc_shim_snd_Pump
#define snd_play_from_global_bank gc_shim_snd_play_from_global_bank
#define snd_send_cmd22 gc_shim_snd_send_cmd22
#define snd_send_cmd23 gc_shim_snd_send_cmd23
#define sprintf gc_shim_sprintf
#define square_root gc_shim_square_root
#define stub_empty_20 gc_shim_stub_empty_20
#define text_draw_centered_small gc_shim_text_draw_centered_small
#define weapon_wheel_init gc_shim_weapon_wheel_init

/* --- The translation layer's own, for the lines the mod adds to the copied files --- */

/* Going Commando's count of frames (60 a second) in the game's ticks: as its PAL build scales its
   press window, (n * 5 + 2) / 6 at 50 a second; n at 60. */
int gc_shim_frames(int frames);
/* The gate's press window (Going Commando's frames) in the game's ticks, as gc_shim_frames, and at
   least long enough to still see the press when the HUD-open request's countdown ends (its start +
   2 polls): with Going Commando's countdown it is the window as it is; the mod's longer "Hold before
   pausing" countdowns would otherwise end after the press has left the window and never open. */
int gc_shim_gate_window(int frames);
/* Ratchet & Clank's commit of item 0x18 (update_hud_item_selector_selection_and_visibility): hero+0x1FF5
   set, no pending item. */
void gc_shim_item_0x18(void);
/* Ratchet & Clank's ring at the selector's alpha (weapon_wheel_draw's lines 44-55). */
void gc_shim_draw_ring(char *menu, int x, int y, int alpha);
/* Ratchet & Clank's own conditions for opening the quick select with Triangle (the gate). */
int gc_shim_selector_open_allowed(void);
/* Ratchet & Clank's skip_frame: nonzero while a level loads. */
int gc_shim_skip_frame(void);

#endif
