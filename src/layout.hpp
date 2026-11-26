// SPDX-License-Identifier: GPL-3.0-or-later
//
// Where Ratchet & Clank keeps what this mod reads, writes and calls, on both discs. PAL from
// games/rac1/decomp (include/names/*.h, include/structs/hero.h, pad.h); NTSC-U from the port's
// translation of the NTSC decomp (games/rac1/ntsc-decomp) and its function table, checked in the
// generated C of OpenRAC at the commit sdk.lock pins. "Level" addresses are level 0's (the "L00"
// labels): every level has its own copy, which Mod::relocate_level(0, ...) finds.
#pragma once

#include <cstdint>

namespace gc_weapon_wheel {

// --- The hero (offsets from g_hero; the same on both discs) ---

// items[0].switch_lock (0x1090 + 0x1C): 2 blocks a change of item.
constexpr std::uint32_t kHeroSwitchLock = 0x10ACu;
// items[0].item_id: the item in hand (Going Commando's hero+0x1248).
constexpr std::uint32_t kHeroItem = 0x10B8u;
// unk1B2 (NTSC: item_select_override_timer): nonzero lets the quick select open over the item locks.
constexpr std::uint32_t kHeroSelectOverride = 0x1B2u;
// unk1FF5: what the quick select sets for item 0x18 in place of a pending item.
constexpr std::uint32_t kHeroItem18 = 0x1FF5u;
// vehicle_mode: 0 on foot, 1 Clank (the four-slot radial), 2 a vehicle, 3 (the quick select's other
// case). Going Commando's mode byte (hero+0x22B4).
constexpr std::uint32_t kHeroVehicleMode = 0x20A4u;
// item_select_blocked and item_use_locked (Going Commando's +0x22B7 and +0x22BC).
constexpr std::uint32_t kHeroSelectBlocked = 0x20A7u;
constexpr std::uint32_t kHeroUseLocked = 0x20ACu;
// pending_item[0]: the item hero_switch_item changes to (Going Commando's hero+0x22C8).
constexpr std::uint32_t kHeroPendingItem = 0x20B8u;
// The item hero_switch_item has taken for slot 0 (its `b[0]`, pending_item + 7 words): the one the
// hand changes to once its animation allows.
constexpr std::uint32_t kHeroRequestedItem = 0x20D4u;
// item_selector_timer (hud_item_selector_close_timer): Going Commando's hero+0x24F6.
constexpr std::uint32_t kHeroCloseTimer = 0x22D2u;
// quick_swap_tap_timer and quick_swap_tap_window: Going Commando's hero+0x24F8 and +0x24FA.
constexpr std::uint32_t kHeroTapTimer = 0x22D4u;
constexpr std::uint32_t kHeroTapWindow = 0x22D6u;

// --- The pad (PadState, include/structs/pad.h) ---
constexpr std::uint32_t kPadHistIdx = 0x18Eu;
constexpr std::uint32_t kPadHistCount = 0x190u;
constexpr std::uint32_t kPadHeld = 0x1A0u;
constexpr std::uint32_t kPadPressed = 0x1A4u;
constexpr std::uint32_t kPadRawReleased = 0x1B8u;
constexpr std::uint32_t kPadHeldUnmasked = 0x1C0u;
constexpr std::uint32_t kPadPressedUnmasked = 0x1C4u;
constexpr std::uint32_t kPadMode = 0x1CCu;
constexpr std::uint32_t kPadStickMoved = 0x1D8u;
constexpr std::uint32_t kPadHistory = 0x1E0u;
constexpr std::uint32_t kPadStickX = 0x148u;
constexpr std::uint32_t kPadStickY = 0x14Cu;
constexpr std::uint32_t kTriangle = 0x10u;

// --- Tables ---
// item_defs: 0x4C a record; +0x38 icon (u16), +0x46 name (s16), +0x48 the gold name (s16).
constexpr std::uint32_t kItemDefStride = 0x4Cu;
constexpr std::uint32_t kItemDefIcon = 0x38u;
constexpr std::uint32_t kItemDefName = 0x46u;
constexpr std::uint32_t kItemDefGoldName = 0x48u;
// item_price_table: 24 bytes a record; +8 uses ammo (u16), +0xE its most ammo (u16).
constexpr std::uint32_t kPriceStride = 24u;
constexpr std::uint32_t kPriceUsesAmmo = 8u;
constexpr std::uint32_t kPriceMaxAmmo = 0xEu;
// usage_stats: 8 bytes an entry (count u16, best u16, level mask u32); 20 the quick select's, 31
// the quick swap's.
constexpr std::uint32_t kUsageStride = 8u;
constexpr int kUsageQuickSelect = 20;
constexpr int kUsageQuickSwap = 31;
// mode_freeze_info: +0 the kind, +0x14 the mode it was entered from.
constexpr std::uint32_t kFreezeInfoKind = 0x0u;
constexpr std::uint32_t kFreezeSavedMode = 0x14u;
// The HUD records (hud_chan_slots, 0x90 bytes each): +0x48 and +0x4A (cleared at init), +0x50 and
// +0x54 the position, +0x58 and +0x5C the size, +0x60 the anchor flags, +0x64 the handle, +0x6C the
// slide, +0x70 the fades, +0x74 the selection, +0x78 the state, +0x7C the timeout.
constexpr std::uint32_t kRecordWidth = 0x58u;
constexpr std::uint32_t kRecordHeight = 0x5Cu;
constexpr std::uint32_t kRecordHandle = 0x64u;
constexpr std::uint32_t kRecordSlide = 0x6Cu;
constexpr std::uint32_t kRecordFades = 0x70u;
constexpr std::uint32_t kRecordSelected = 0x74u;
// HUD sprite ids: the quick select's panel quarter and its selection marker.
constexpr int kPanelSprite = 0xE934;
constexpr int kMarkerSprite = 0xE99C;

struct Disc {
    bool ntsc;
    // --- The executable's globals (Mod::relocate: a level's program keeps some of them elsewhere,
    // the game mode among them) ---
    std::uint32_t hero;               // g_hero
    std::uint32_t pad;                // g_controller_state
    std::uint32_t slot_items;         // the slot assignment, int[8]
    std::uint32_t ammo;               // weapon_ammo_counts
    std::uint32_t gold;               // the gold weapons' flags (item_text_variant), u8 per item
    std::uint32_t usage_stats;        // usage_stats
    std::uint32_t mode_freeze_info;   // mode_freeze_info
    std::uint32_t bolts;              // gBolts
    std::uint32_t quick_swap_item;    // quick_swap_item
    std::uint32_t last_weapon;        // last_weapon_id
    std::uint32_t level_index;        // g_current_level_index
    std::uint32_t event_frames;       // gameplay_event_frame_count
    std::uint32_t game_mode;          // g_game_mode (mode_freeze_init's and update_mode_freeze's)
    std::uint32_t frame_scale;        // frame_scale, scale_ticks' factor (float)
    // --- Level 0's data ---
    std::uint32_t item_defs;
    std::uint32_t item_price;
    std::uint32_t slot_tables;        // hud_selector_slot_tables: the page table's address
    std::uint32_t page;               // quick_select_page
    std::uint32_t slot_count;         // quick_select_slot_count
    std::uint32_t page0_entries;      // the eight-slot selector's entries
    std::uint32_t default_left;       // quick_select_default_left_slot (right, up, down follow)
    std::uint32_t cinematic_mode;     // the game mode as the level's main loop reads it
    std::uint32_t cinematic_frames;   // ticks since it changed
    std::uint32_t level_frame_counter;
    std::uint32_t skip_frame;
    std::uint32_t draw_mask;          // debug_profiler_color
    std::uint32_t rendered_frame_count;
    std::uint32_t timed_hud_visible;
    std::uint32_t angle_offset;       // quick_select_slot_angle_offset
    std::uint32_t saved_selected;     // the selection a close leaves
    std::uint32_t saved_state;        // quick_select_last_state
    std::uint32_t close_frame;        // quick_select_close_frame
    std::uint32_t ammo_format;        // hud_ammo_format
    std::uint32_t width;              // quick_select_width, quick_select_height
    std::uint32_t height;
    std::uint32_t center_x;           // quick_select_center_x, quick_select_center_y
    std::uint32_t center_y;
    std::uint32_t ring_radius;        // float
    std::uint32_t ring_aspect;        // float
    std::uint32_t unselected_alpha;   // float
    std::uint32_t pulse_period;       // quick_select_pulse_period: the marker's pulse, in ticks
};

inline Disc disc_for(bool ntsc) {
    Disc d{};
    d.ntsc = ntsc;
    if (ntsc) {
        d.hero = 0x0013F350u;
        d.pad = 0x0013C940u;
        d.slot_items = 0x00141EA0u;
        d.ammo = 0x0013D428u;
        d.gold = 0x0013E520u;
        d.usage_stats = 0x00141848u;
        d.mode_freeze_info = 0x00193300u;
        d.bolts = 0x0015ED98u;
        d.quick_swap_item = 0x0015ED8Cu;
        d.last_weapon = 0x00141660u;
        d.level_index = 0x0015ED84u;
        d.event_frames = 0x0015EEA4u;
        d.game_mode = 0x0015F604u;
        d.frame_scale = 0x0015ED60u;
        d.item_defs = 0x00179AC0u;
        d.item_price = 0x001C40B0u;
        d.slot_tables = 0x0015FAC0u;
        d.page = 0x0015F72Cu;
        d.slot_count = 0x0015FA90u;
        d.page0_entries = 0x0017DAC0u;
        d.default_left = 0x0017DC18u;
        d.cinematic_mode = 0x0015F5C4u;
        d.cinematic_frames = 0x0015F5C8u;
        d.level_frame_counter = 0x0015F5CCu;
        d.skip_frame = 0x0015F5D8u;
        d.draw_mask = 0x0015F3F4u;
        d.rendered_frame_count = 0x0015F3F8u;
        d.timed_hud_visible = 0x0015F594u;
        d.angle_offset = 0x0015F784u;
        d.saved_selected = 0x0015FA9Cu;
        d.saved_state = 0x0015FA98u;
        d.close_frame = 0x0015FA94u;
        d.ammo_format = 0x0015F7C8u;
        d.width = 0x0015F718u;
        d.height = 0x0015F71Cu;
        d.center_x = 0x0015F720u;
        d.center_y = 0x0015F724u;
        d.ring_radius = 0x0015F78Cu;
        d.ring_aspect = 0x0015F7C4u;
        d.unselected_alpha = 0x0015F7C0u;
        d.pulse_period = 0x0015F7B0u;
    } else {
        d.hero = 0x0013F450u;
        d.pad = 0x0013CA40u;
        d.slot_items = 0x00141FA0u;
        d.ammo = 0x0013D530u;
        d.gold = 0x0013E620u;
        d.usage_stats = 0x00141948u;
        d.mode_freeze_info = 0x00193400u;
        d.bolts = 0x0015EE98u;
        d.quick_swap_item = 0x0015EE8Cu;
        d.last_weapon = 0x00141760u;
        d.level_index = 0x0015EE84u;
        d.event_frames = 0x0015EFA4u;
        d.game_mode = 0x0015F6E8u;
        d.frame_scale = 0x0015EE60u;
        d.item_defs = 0x00179BC0u;
        d.item_price = 0x001C43B0u;
        d.slot_tables = 0x0015FB78u;
        d.page = 0x0015F80Cu;
        d.slot_count = 0x0015FB48u;
        d.page0_entries = 0x0017DBC0u;
        d.default_left = 0x0017DD18u;
        d.cinematic_mode = 0x0015F6A8u;
        d.cinematic_frames = 0x0015F6ACu;
        d.level_frame_counter = 0x0015F6B0u;
        d.skip_frame = 0x0015F6BCu;
        d.draw_mask = 0x0015F4F4u;
        d.rendered_frame_count = 0x0015F4F8u;
        d.timed_hud_visible = 0x0015F674u;
        d.angle_offset = 0x0015F864u;
        d.saved_selected = 0x0015FB54u;
        d.saved_state = 0x0015FB50u;
        d.close_frame = 0x0015FB4Cu;
        d.ammo_format = 0x0015F8A8u;
        d.width = 0x0015F7F8u;
        d.height = 0x0015F7FCu;
        d.center_x = 0x0015F800u;
        d.center_y = 0x0015F804u;
        d.ring_radius = 0x0015F86Cu;
        d.ring_aspect = 0x0015F8A4u;
        d.unselected_alpha = 0x0015F8A0u;
        d.pulse_period = 0x0015F890u;
    }
    return d;
}

// --- The HUD callbacks queue_animation_update takes: each level's own copy of the quick select's
// init, update and draw and the bolt counter's, at that level's code addresses (the decomps'
// catalogues: PAL games/rac1/decomp config/overlays/functions.tsv, NTSC games/rac1/ntsc-decomp
// config/us/levels/functions.tsv, "places"). Level programs 0 to 18. ---
constexpr int kLevels = 19;
struct LevelCallbacks {
    std::uint32_t selector_init, selector_update, selector_draw;
    std::uint32_t bolt_init, bolt_update, bolt_draw;
};
// Their names in the disc's function table (every level's copy is listed under level 0's name).
inline constexpr const char* kCallbackNamesPal[6] = {"func_L00_00238478", "func_L00_00238530", "func_L00_00238C30",
                                                     "func_L00_0023A658", "func_L00_0023A690", "func_L00_0023A788"};
inline constexpr const char* kCallbackNamesNtsc[6] = {"FUN_L00_00237ae8", "FUN_L00_00237ba0", "FUN_L00_002382a0",
                                                      "FUN_L00_00239cc8", "FUN_L00_00239d00", "FUN_L00_00239df8"};
inline constexpr LevelCallbacks kCallbacksPal[kLevels] = {
    {0x00238478u, 0x00238530u, 0x00238C30u, 0x0023A658u, 0x0023A690u, 0x0023A788u},  // 0
    {0x0024DA88u, 0x0024DB40u, 0x0024E240u, 0x0024F1D8u, 0x0024F210u, 0x0024F308u},  // 1
    {0x0023B0D8u, 0x0023B190u, 0x0023B890u, 0x0023C828u, 0x0023C860u, 0x0023C958u},  // 2
    {0x00227BB0u, 0x00227C68u, 0x00228368u, 0x00229300u, 0x00229338u, 0x00229430u},  // 3
    {0x0022B9D8u, 0x0022BA90u, 0x0022C190u, 0x0022DBB8u, 0x0022DBF0u, 0x0022DCE8u},  // 4
    {0x00261808u, 0x002618C0u, 0x00261FC0u, 0x00262F58u, 0x00262F90u, 0x00263088u},  // 5
    {0x00247970u, 0x00247A28u, 0x00248128u, 0x00249B50u, 0x00249B88u, 0x00249C80u},  // 6
    {0x00260260u, 0x00260318u, 0x00260A18u, 0x00262440u, 0x00262478u, 0x00262570u},  // 7
    {0x00243660u, 0x00243718u, 0x00243E18u, 0x00244DB0u, 0x00244DE8u, 0x00244EE0u},  // 8
    {0x00258058u, 0x00258110u, 0x00258810u, 0x0025A238u, 0x0025A270u, 0x0025A368u},  // 9
    {0x0022BA60u, 0x0022BB18u, 0x0022C218u, 0x0022DC40u, 0x0022DC78u, 0x0022DD70u},  // 10
    {0x0025EE18u, 0x0025EED0u, 0x0025F5D0u, 0x00260568u, 0x002605A0u, 0x00260698u},  // 11
    {0x00251C68u, 0x00251D20u, 0x00252420u, 0x002533B8u, 0x002533F0u, 0x002534E8u},  // 12
    {0x00243EE0u, 0x00243F98u, 0x00244698u, 0x002460C0u, 0x002460F8u, 0x002461F0u},  // 13
    {0x00240EE0u, 0x00240F98u, 0x00241698u, 0x00242630u, 0x00242668u, 0x00242760u},  // 14
    {0x00229000u, 0x002290B8u, 0x002297B8u, 0x0022A750u, 0x0022A788u, 0x0022A880u},  // 15
    {0x00233E88u, 0x00233F40u, 0x00234640u, 0x002355D8u, 0x00235610u, 0x00235708u},  // 16
    {0x002337D8u, 0x00233890u, 0x00233F90u, 0x002359B8u, 0x002359F0u, 0x00235AE8u},  // 17
    {0x0023B9E8u, 0x0023BAA0u, 0x0023C1A0u, 0x0023D138u, 0x0023D170u, 0x0023D268u},  // 18
};
inline constexpr LevelCallbacks kCallbacksNtsc[kLevels] = {
    {0x00237AE8u, 0x00237BA0u, 0x002382A0u, 0x00239CC8u, 0x00239D00u, 0x00239DF8u},  // 0
    {0x0024D180u, 0x0024D238u, 0x0024D938u, 0x0024E8D0u, 0x0024E908u, 0x0024EA00u},  // 1
    {0x0023A7A8u, 0x0023A860u, 0x0023AF60u, 0x0023BEF8u, 0x0023BF30u, 0x0023C028u},  // 2
    {0x002272B8u, 0x00227370u, 0x00227A70u, 0x00228A08u, 0x00228A40u, 0x00228B38u},  // 3
    {0x0022B0C8u, 0x0022B180u, 0x0022B880u, 0x0022D2A8u, 0x0022D2E0u, 0x0022D3D8u},  // 4
    {0x00260E60u, 0x00260F18u, 0x00261618u, 0x002625B0u, 0x002625E8u, 0x002626E0u},  // 5
    {0x00247010u, 0x002470C8u, 0x002477C8u, 0x002491F0u, 0x00249228u, 0x00249320u},  // 6
    {0x0025F950u, 0x0025FA08u, 0x00260108u, 0x00261B30u, 0x00261B68u, 0x00261C60u},  // 7
    {0x00242D58u, 0x00242E10u, 0x00243510u, 0x002444A8u, 0x002444E0u, 0x002445D8u},  // 8
    {0x002577C8u, 0x00257880u, 0x00257F80u, 0x002599A8u, 0x002599E0u, 0x00259AD8u},  // 9
    {0x0022B150u, 0x0022B208u, 0x0022B908u, 0x0022D330u, 0x0022D368u, 0x0022D460u},  // 10
    {0x0025E518u, 0x0025E5D0u, 0x0025ECD0u, 0x0025FC68u, 0x0025FCA0u, 0x0025FD98u},  // 11
    {0x00251358u, 0x00251410u, 0x00251B10u, 0x00252AA8u, 0x00252AE0u, 0x00252BD8u},  // 12
    {0x002435D0u, 0x00243688u, 0x00243D88u, 0x002457B0u, 0x002457E8u, 0x002458E0u},  // 13
    {0x00240550u, 0x00240608u, 0x00240D08u, 0x00241CA0u, 0x00241CD8u, 0x00241DD0u},  // 14
    {0x002286E0u, 0x00228798u, 0x00228E98u, 0x00229E30u, 0x00229E68u, 0x00229F60u},  // 15
    {0x00233570u, 0x00233628u, 0x00233D28u, 0x00234CC0u, 0x00234CF8u, 0x00234DF0u},  // 16
    {0x00232DD0u, 0x00232E88u, 0x00233588u, 0x00234FB0u, 0x00234FE8u, 0x002350E0u},  // 17
    {0x0023B0C8u, 0x0023B180u, 0x0023B880u, 0x0023C818u, 0x0023C850u, 0x0023C948u},  // 18
};

// Level `level`'s callbacks; all 0 for a level without a program of its own.
inline LevelCallbacks level_callbacks(bool ntsc, int level) {
    if (level < 0 || level >= kLevels) {
        return {};
    }
    return ntsc ? kCallbacksNtsc[level] : kCallbacksPal[level];
}

// Ratchet & Clank's functions by their names in each disc's build: PAL's "func_<address>"
// (games/rac1/decomp's names.h), NTSC's as the port's function table has them.
struct FunctionNames {
    const char* mode_freeze_init;
    const char* update_hud_elements;
    const char* help_update;
    const char* music_unpause;
    const char* music_pause;
    const char* snd_pause_group;
    const char* snd_continue_group;
    const char* snd_flush;
    const char* draw_hud_sprite;
    const char* get_icon_frame;
    const char* hud_frame;
    const char* font_print_center;
    const char* msg_string;
    const char* fast_tween_color;
    const char* hud_screen_position;
    const char* show_health_hud;
    const char* hud_keep_visible;
    const char* queue_animation_update;
    const char* remove_hud_item;
    const char* append_gif_transfer_packet;
    const char* draw_debug_profiler;
    const char* play_bank_sound;
    // The math the copied code does, Ratchet & Clank's own (optional: a faithful C stand-in
    // otherwise).
    const char* square_root;
    const char* fast_arc_tan;
    const char* diff_rots;
    const char* sub_rots;
    const char* normalize_angle;
    const char* add_rots;
    const char* sin;
    const char* cos;
    const char* int_to_float;
    const char* float_to_int;
};

inline FunctionNames function_names(bool ntsc) {
    if (ntsc) {
        return {"FUN_001fbab8", "FUN_L00_0023ac78", "FUN_001fde90", "music_unpause", "music_pause",
                "FUN_0012e3e8", "FUN_0012e418", "FUN_0012dc80", "FUN_001ffc30", "FUN_001ff960",
                "FUN_L00_0023b120", "FUN_001f6af0", "FUN_001fdd10", "FUN_001fa6e0", "FUN_L00_00235a70",
                "FUN_L00_00234e00", "FUN_L00_00235e18", "FUN_001ff308", "FUN_001ff480", "FUN_001fb368",
                "FUN_001f39d0", "FUN_0022db10",
                "FUN_001f9988", "FUN_001f9e90", "FUN_001fa688", "FUN_001fa5c8", "FUN_001fa610",
                "FUN_001fa580", "FUN_001f9de0", "FUN_001f9dc8", "func_001FA6C0", "FUN_001fa6d0"};
    }
    return {"func_001FBC80", "func_L00_0023B610", "func_001FE6C0", "func_00216F28", "func_00216EF0",
            "func_0012E528", "func_0012E558", "func_0012DDC0", "func_00200468", "func_00200198",
            "func_L00_0023BAB8", "func_001F6EA8", "func_001FE540", "func_001FA8A8", "func_L00_00236400",
            "func_L00_00235790", "func_L00_002367A8", "func_001FFB38", "func_001FFCB0", "func_001FB530",
            "func_001F3D78", "func_0022EE28",
            "func_001F9B50", "func_001FA058", "func_001FA850", "func_001FA790", "func_001FA7D8",
            "func_001FA748", "func_001F9FA8", "func_001F9F90", "func_001FA888", "func_001FA898"};
}

}  // namespace gc_weapon_wheel
