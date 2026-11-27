// SPDX-License-Identifier: GPL-3.0-or-later
//
// The translation layer (rac1_shim.hpp says what it is): the buffers and pointers the copied
// Going Commando code works on, the hook points' bodies, and the polyfills the copied code calls
// in place of Going Commando's functions. Each polyfill says what it is in Going Commando
// (rac2-decomp at 4ea93cea, its file), what stands in for it here, and what differs.
#include "rac1_shim.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace gcw = gc_weapon_wheel;
using gc_weapon_wheel::gc::CopyScope;
using gc_weapon_wheel::gc::Shim;
using gc_weapon_wheel::gc::shim;

// --- The buffers and pointers the copied code works on (gc/shim/common.h names them) ---

extern "C" {
// Pointers into game memory: set by connect() and before each call of copied code.
void* gc_shim_pad;
void* gc_shim_page_index;
void* gc_shim_slot_count;
void* gc_shim_default_choices;
void* gc_shim_angle_delta;
void* gc_shim_saved_selected;
void* gc_shim_saved_state;
void* gc_shim_close_frame;
void* gc_shim_rendered_frame_count;
void* gc_shim_slot_items;
void* gc_shim_page0_entries;
void* gc_shim_ammo;
void* gc_shim_ammo_format;
void* gc_shim_wheel_timer;
void* gc_shim_level_index;
void* gc_shim_game_mode;
void* gc_shim_cinematic_mode;
void* gc_shim_cinematic_frames;
void* gc_shim_draw_mask;
void* gc_shim_rac1_page_table;
// The page table's game address as a pointer: weapon_menu_init_slots only takes its address
// (`(u32)&g_radial_pages_data`), which is this value; never dereferenced.
void* gc_shim_rac1_pages_data;

// The copies, at Going Commando's offsets.
alignas(16) unsigned char gc_shim_hero[gc_weapon_wheel::gc::kHeroBytes];
alignas(16) unsigned char gc_shim_stats[gc_weapon_wheel::gc::kStatsBytes];
alignas(16) unsigned char gc_shim_weapon_table[gc_weapon_wheel::gc::kWeapons * gc_weapon_wheel::gc::kWeaponRecord];
unsigned char gc_shim_item_record[256];
std::uint32_t gc_shim_frame_counter;
// Host pointers to each page's entries (gc_shim_menu_pages points here).
void* gc_shim_pages[gc_weapon_wheel::gc::kPages];
void** gc_shim_menu_pages = gc_shim_pages;

// The mod's own.
alignas(16) unsigned char gc_shim_modal_state[gc_weapon_wheel::gc::kModalBytes];
void* gc_shim_menu_saved_state = gc_shim_modal_state + gc_weapon_wheel::gc::kModalMode;
void* gc_shim_menu_second_value = gc_shim_modal_state + gc_weapon_wheel::gc::kModalWorldDrawn;
void* gc_shim_screen_mode_flag = gc_shim_hero + gc_weapon_wheel::gc::kGcHeroMode;
void* gc_shim_hold_timers = gc_shim_hero + gc_weapon_wheel::gc::kGcHeroTapTimer;
std::int32_t gc_shim_modal_state_word[4];
int gc_shim_menu_mode;
int gc_shim_vis_fallback_mode;
int gc_shim_modal_counter;
// UpdateModalState's start of the counter (ref_00286738_00): 6 frames in Going Commando's data
// (rac2-decomp's note on func_00286738); set in the game's ticks by Shim::reset.
int gc_shim_modal_counter_start;
int gc_shim_modal_clear_flag;
int gc_shim_modal_timer;
// hud_request_letterbox_bars' flag: Going Commando's bars are left out (README), nothing reads it.
int gc_shim_letterbox_grow_flag;
// The HUD-open request (gc/shim/common.h): its state (_b), the double tap's swap flag (_a), the
// countdown and its start (4 frames in Going Commando's data, in the game's ticks: Shim::reset).
int gc_shim_hud_flag_a;
int gc_shim_hud_flag_b;
int gc_shim_open_delay_start;
int gc_shim_open_delay_counter;
// The pad's held word (pad + 0x1A0), which the request's state machine tests.
void* gc_shim_pad_held;
int gc_shim_press_window;
// Going Commando's input block (D_L00_001A8FB4: its hit streak's): Ratchet & Clank has none.
int gc_shim_input_block;
// Going Commando's notice owner (a notice on the screen holds the quick select): none here.
int gc_shim_notice_owner;
// Going Commando's resident HUD data: Ratchet & Clank's HUD is always there. Read as an int by one
// file and as a pointer by another (whose ring lines are cut), so pointer-sized, little-endian 1.
std::uintptr_t gc_shim_resident_data_base = 1;
int gc_shim_wheel_fade_count;
// Going Commando's sequence flag (a scripted sequence holds the commit): Ratchet & Clank's quick
// select has no such check.
short gc_shim_sequence_active;
char gc_shim_str_wheel_debug[1];
// Going Commando's option bytes (D_001A7BB0): [3] is "Pause while open".
unsigned char gc_shim_options[8];
}

namespace {

namespace gc = gc_weapon_wheel::gc;
using gc_weapon_wheel::Disc;

constexpr float kPi = 3.1415927f;
// Going Commando's state 0x37 (hero_update_button_hold_timers' early return): unknown here, and
// Ratchet & Clank's own double tap has no such check, so the copy's state never matches it.
constexpr std::int32_t kNoGcState = -1;

gc::Memory& mem() { return shim().game.memory; }

std::uint8_t* host(std::uint32_t address) { return mem().at(address); }

// The record's game address from the host pointer the copied code holds.
std::uint32_t guest(const void* p) { return mem().address_of(p); }


}  // namespace

namespace gc_weapon_wheel::gc {

Shim& shim() {
    static Shim s;
    return s;
}

void Shim::set_options(bool pause, bool click, int open_delay_frames) {
    m_pause = pause;
    m_click = click;
    m_open_delay = open_delay_frames;
    // Only the countdown's start (the copied state machine reads it each request), in the game's
    // ticks as Going Commando's PAL build scales its frame counts.
    gc_shim_open_delay_start = frames(m_open_delay);
    gc_shim_options[3] = pause ? 1 : 0;
}

bool Shim::pause() const { return gc_shim_options[3] != 0; }

void Shim::reset() {
    eqs_end();
    m_copy_depth = 0;
    m_entered = false;
    m_refused.clear();
    m_freeze_ticks = 0;
    m_draws_since_update = 0;
    m_record_opened = false;
    std::memset(gc_shim_modal_state, 0, sizeof gc_shim_modal_state);
    std::memset(gc_shim_hero, 0, sizeof gc_shim_hero);
    std::memset(gc_shim_stats, 0, sizeof gc_shim_stats);
    std::memset(gc_shim_pages, 0, sizeof gc_shim_pages);
    gc_shim_modal_counter = 0;
    gc_shim_modal_timer = 0;
    gc_shim_modal_clear_flag = 0;
    gc_shim_letterbox_grow_flag = 0;
    gc_shim_hud_flag_a = 0;
    gc_shim_hud_flag_b = 0;
    gc_shim_open_delay_counter = 0;
    gc_shim_open_delay_start = frames(m_open_delay);
    m_request_tick = 0;
    m_request_trace.clear();
    gc_shim_press_window = 0;
    gc_shim_modal_counter_start = frames(6);
    for (int i = 0; i < 256; ++i) {
        gc_shim_item_record[i] = static_cast<unsigned char>(i);
    }
    gc_shim_options[3] = m_pause ? 1 : 0;
}

void Shim::log_once(const std::string& key, const std::string& line, bool warning) {
    if (std::find(m_logged.begin(), m_logged.end(), key) != m_logged.end()) {
        return;
    }
    m_logged.push_back(key);
    const auto& out = warning ? game.log : game.info;
    if (out) {
        out(line.c_str());
    }
}

void Shim::connect(int level) {
    if (level == m_level_number) {
        return;
    }
    m_level_number = level;
    reset();
    const Disc& d = game.disc;
    LevelAddresses l;
    l.at = d;
    auto rel = [this](std::uint32_t a) { return game.relocate ? game.relocate(a) : a; };
    auto exe = [this](std::uint32_t a) { return game.relocate_exe ? game.relocate_exe(a) : a; };
    // The executable's globals where the program loaded keeps them (OPENRAC_DATA): the game mode
    // mode_freeze_init and update_mode_freeze write is one a level's program keeps elsewhere.
    for (std::uint32_t Disc::*f :
         {&Disc::hero, &Disc::pad, &Disc::slot_items, &Disc::ammo, &Disc::gold, &Disc::usage_stats,
          &Disc::mode_freeze_info, &Disc::bolts,
          &Disc::quick_swap_item, &Disc::last_weapon, &Disc::level_index, &Disc::event_frames, &Disc::game_mode,
          &Disc::frame_scale}) {
        l.at.*f = exe(d.*f);
    }
    if (level >= 0) {
        for (std::uint32_t Disc::*f :
             {&Disc::item_defs, &Disc::item_price, &Disc::slot_tables, &Disc::page, &Disc::slot_count,
              &Disc::page0_entries, &Disc::default_left, &Disc::cinematic_mode, &Disc::cinematic_frames,
              &Disc::level_frame_counter, &Disc::skip_frame, &Disc::draw_mask, &Disc::rendered_frame_count,
              &Disc::timed_hud_visible, &Disc::angle_offset, &Disc::saved_selected, &Disc::saved_state,
              &Disc::close_frame, &Disc::ammo_format, &Disc::width, &Disc::height, &Disc::center_x,
              &Disc::center_y, &Disc::ring_radius, &Disc::ring_aspect, &Disc::unselected_alpha,
              &Disc::pulse_period}) {
            l.at.*f = rel(d.*f);
        }
        // The callbacks: this level's own copies (the decomps' catalogues), each taken only when the
        // disc's function table has the function of that name there.
        const LevelCallbacks places = level_callbacks(d.ntsc, level);
        const char* const* names = d.ntsc ? kCallbackNamesNtsc : kCallbackNamesPal;
        const std::uint32_t found[6] = {places.selector_init, places.selector_update, places.selector_draw,
                                        places.bolt_init, places.bolt_update, places.bolt_draw};
        bool ok[6];
        std::string report;
        for (int i = 0; i < 6; ++i) {
            ok[i] = found[i] != 0 && (!game.code_matches || game.code_matches(found[i], names[i]));
            char part[64];
            std::snprintf(part, sizeof part, "%s%s at 0x%08X %s", i ? ", " : "", names[i], found[i],
                          found[i] == 0 ? "(no place)" : ok[i] ? "ok" : "NOT that function");
            report += part;
        }
        if (ok[0] && ok[1] && ok[2]) {
            l.selector_init = found[0];
            l.selector_update = found[1];
            l.selector_draw = found[2];
        }
        if (ok[3] && ok[4] && ok[5]) {
            l.bolt_init = found[3];
            l.bolt_update = found[4];
            l.bolt_draw = found[5];
        }
        const bool all = ok[0] && ok[1] && ok[2] && ok[3] && ok[4] && ok[5];
        char head[96];
        std::snprintf(head, sizeof head, "level %d: the HUD callbacks %s: ", level,
                      all ? "found" : "NOT all found (the paused open needs the quick select's three)");
        log_once("callbacks " + std::to_string(level), head + report, !all);
        // The game mode as mode_freeze_init writes it and as the level's main loop reads it: one word.
        const std::uint32_t loop = rel(d.cinematic_mode);
        char modes[160];
        std::snprintf(modes, sizeof modes, "level %d: the game mode word 0x%08X (the executable's 0x%08X relocated); "
                      "the main loop's 0x%08X (level 0's 0x%08X relocated)%s", level, l.at.game_mode, d.game_mode, loop,
                      d.cinematic_mode, loop == l.at.game_mode ? "" : ": NOT the same word");
        log_once("modes " + std::to_string(level), modes, loop != l.at.game_mode);
    }
    m_level = l;

    const Disc& a = m_level.at;
    gc_shim_pad = host(a.pad);
    gc_shim_pad_held = host(a.pad + kPadHeld);
    gc_shim_slot_items = host(a.slot_items);
    gc_shim_ammo = host(a.ammo);
    gc_shim_wheel_timer = host(a.hero + kHeroCloseTimer);
    gc_shim_level_index = host(a.level_index);
    gc_shim_game_mode = host(a.game_mode);
    gc_shim_page_index = host(a.page);
    gc_shim_slot_count = host(a.slot_count);
    // g_wheel_default_choices[2..5] are left, right, up, down: the pointer is two words before left.
    gc_shim_default_choices = host(a.default_left - 8);
    gc_shim_angle_delta = host(a.angle_offset);
    gc_shim_saved_selected = host(a.saved_selected);
    gc_shim_saved_state = host(a.saved_state);
    gc_shim_close_frame = host(a.close_frame);
    gc_shim_rendered_frame_count = host(a.rendered_frame_count);
    gc_shim_page0_entries = host(a.page0_entries);
    gc_shim_ammo_format = host(a.ammo_format);
    gc_shim_cinematic_mode = host(a.cinematic_mode);
    gc_shim_cinematic_frames = host(a.cinematic_frames);
    gc_shim_draw_mask = host(a.draw_mask);
    gc_shim_rac1_page_table = host(a.slot_tables);
    // Ratchet & Clank's page table: quick_select_page + 12 (initialize_eight_slot_hud_selector:
    // hud_selector_slot_tables = &quick_select_page + 6, a short pointer).
    gc_shim_rac1_pages_data = reinterpret_cast<void*>(static_cast<std::uintptr_t>(a.page + 12));
}

int Shim::ticks(int n) const {
    float scale = game.memory.base != nullptr ? game.memory.read<float>(m_level.at.frame_scale) : 0.0f;
    // frame_scale's NTSC address is the port's guess (rac1_version.h: "not checked"): a value out of
    // range takes the tick rate's own factor.
    if (!(scale >= 0.5f && scale <= 1.5f)) {
        scale = game.disc.ntsc ? 1.0f : 50.0f / 60.0f;
    }
    return static_cast<int>(0.5f + static_cast<float>(n) * scale);
}

int Shim::frames(int n) const { return game.disc.ntsc ? n : (n * 5 + 2) / 6; }

bool Shim::freeze_kind_is_ours() const {
    return game.memory.read<std::int32_t>(m_level.at.mode_freeze_info + kFreezeInfoKind) == kFreezeKind;
}

bool Shim::in_wheel_freeze() const {
    std::int32_t mode = 0;
    std::memcpy(&mode, gc_shim_modal_state + kModalMode, sizeof mode);
    return game.memory.read<std::int32_t>(m_level.at.game_mode) == kModeFreeze && freeze_kind_is_ours() &&
           mode == kModalQuickSelect;
}

// --- The copies ---

void Shim::load_copies() {
    const Memory& m = game.memory;
    const Disc& a = m_level.at;
    Memory copy{gc_shim_hero};
    copy.write<std::int32_t>(kGcHeroItem, m.read<std::int32_t>(a.hero + kHeroItem));
    copy.write<std::int32_t>(kGcHeroPending, m.read<std::int32_t>(a.hero + kHeroPendingItem));
    copy.write<std::int32_t>(kGcHeroState, kNoGcState);
    copy.write<std::uint8_t>(kGcHeroMode, m.read<std::uint8_t>(a.hero + kHeroVehicleMode));
    copy.write<std::uint8_t>(kGcHeroUseLocked, m.read<std::uint8_t>(a.hero + kHeroUseLocked));
    copy.write<std::int16_t>(kGcHeroTapTimer, m.read<std::int16_t>(a.hero + kHeroTapTimer));
    copy.write<std::int16_t>(kGcHeroTapWindow, m.read<std::int16_t>(a.hero + kHeroTapWindow));
    copy.write<std::int32_t>(kGcHeroSwapRequest, 0);
    m_seen.pending = copy.read<std::int32_t>(kGcHeroPending);
    m_seen.tap_timer = copy.read<std::int16_t>(kGcHeroTapTimer);
    m_seen.tap_window = copy.read<std::int16_t>(kGcHeroTapWindow);

    // The quick select's usage entry: Ratchet & Clank's count, best and levels at Going Commando's
    // +0xF0, +0xF4, +0xF8; the "frame counter" Going Commando compares the best with is the value
    // Ratchet & Clank's own entry keeps (scale_ticks(gameplay_event_frame_count) / 600).
    Memory stats{gc_shim_stats};
    const std::uint32_t entry = a.usage_stats + kUsageStride * kUsageQuickSelect;
    stats.write<std::uint16_t>(kGcStatsQuickSelect, m.read<std::uint16_t>(entry));
    stats.write<std::uint32_t>(kGcStatsQuickSelect + 4, m.read<std::uint16_t>(entry + 2));
    stats.write<std::uint32_t>(kGcStatsQuickSelect + 8, m.read<std::uint32_t>(entry + 4));
    m_seen.count = stats.read<std::uint16_t>(kGcStatsQuickSelect);
    m_seen.best = stats.read<std::uint32_t>(kGcStatsQuickSelect + 4);
    m_seen.levels = stats.read<std::uint32_t>(kGcStatsQuickSelect + 8);
    gc_shim_frame_counter =
        static_cast<std::uint32_t>(ticks(m.read<std::int32_t>(a.event_frames)) / 600);
}

void Shim::flush_copies() {
    const Memory& m = game.memory;
    const Disc& a = m_level.at;
    Memory copy{gc_shim_hero};
    const std::int32_t pending = copy.read<std::int32_t>(kGcHeroPending);
    if (pending != m_seen.pending) {
        m.write<std::int32_t>(a.hero + kHeroPendingItem, pending);
    }
    const std::int16_t tap = copy.read<std::int16_t>(kGcHeroTapTimer);
    if (tap != m_seen.tap_timer) {
        m.write<std::int16_t>(a.hero + kHeroTapTimer, tap);
    }
    const std::int16_t window = copy.read<std::int16_t>(kGcHeroTapWindow);
    if (window != m_seen.tap_window) {
        m.write<std::int16_t>(a.hero + kHeroTapWindow, window);
    }
    Memory stats{gc_shim_stats};
    const std::uint32_t entry = a.usage_stats + kUsageStride * kUsageQuickSelect;
    if (stats.read<std::uint16_t>(kGcStatsQuickSelect) != m_seen.count) {
        m.write<std::uint16_t>(entry, stats.read<std::uint16_t>(kGcStatsQuickSelect));
    }
    const std::uint32_t best = stats.read<std::uint32_t>(kGcStatsQuickSelect + 4);
    if (best != m_seen.best) {
        m.write<std::uint16_t>(entry + 2, static_cast<std::uint16_t>(std::min<std::uint32_t>(best, 0xFFFFu)));
    }
    const std::uint32_t levels = stats.read<std::uint32_t>(kGcStatsQuickSelect + 8);
    if (levels != m_seen.levels) {
        m.write<std::uint32_t>(entry + 4, levels);
    }
}

// Going Commando's weapon table, one 0xE0-byte record per Ratchet & Clank item, for the items the
// quick select shows: +0x00 "active" (1: Ratchet & Clank shows any item's ammo that uses ammo),
// +0x14 its class (0: no class preload here), +0x3C its icon and +0x48 its name (item_defs: the gold
// name for a gold weapon, as Ratchet & Clank's draw picks it), +0x88 uses ammo and +0x8E its most
// (item_price_table).
void Shim::fill_weapon_table() {
    const Memory& m = game.memory;
    const Disc& a = m_level.at;
    auto fill = [&](std::int32_t id) {
        if (id <= 0 || static_cast<std::size_t>(id) >= kWeapons) {
            return;
        }
        Memory r{gc_shim_weapon_table + static_cast<std::size_t>(id) * kWeaponRecord};
        std::memset(r.base, 0, kWeaponRecord);
        const std::uint32_t def = a.item_defs + static_cast<std::uint32_t>(id) * kItemDefStride;
        const std::uint32_t price = a.item_price + static_cast<std::uint32_t>(id) * kPriceStride;
        const bool gold = m.read<std::uint8_t>(a.gold + static_cast<std::uint32_t>(id)) != 0;
        r.write<std::int32_t>(0x00, 1);
        r.write<std::uint16_t>(0x3C, m.read<std::uint16_t>(def + kItemDefIcon));
        r.write<std::int16_t>(0x48, m.read<std::int16_t>(def + (gold ? kItemDefGoldName : kItemDefName)));
        r.write<std::uint16_t>(0x88, m.read<std::uint16_t>(price + kPriceUsesAmmo));
        r.write<std::uint16_t>(0x8E, m.read<std::uint16_t>(price + kPriceMaxAmmo));
    };
    for (std::uint32_t i = 0; i < 8; ++i) {
        fill(m.read<std::int32_t>(a.slot_items + 4 * i));
    }
    for (std::size_t p = 0; p < kPages; ++p) {
        if (gc_shim_pages[p] == nullptr) {
            continue;
        }
        const Memory entries{static_cast<std::uint8_t*>(gc_shim_pages[p])};
        for (std::uint32_t i = 0; i < 8; ++i) {
            fill(entries.read<std::int32_t>(i * 28 + 0x18));
        }
    }
}

// Going Commando's page table holds host pointers; Ratchet & Clank's (hud_selector_slot_tables)
// points at an array of 32-bit game addresses, each a page of seven-word entries.
void Shim::rebuild_pages() {
    const Memory& m = game.memory;
    const std::uint32_t table = m.read<std::uint32_t>(m_level.at.slot_tables);
    for (std::size_t p = 0; p < kPages; ++p) {
        const std::uint32_t page = table != 0 ? m.read<std::uint32_t>(table + 4 * static_cast<std::uint32_t>(p)) : 0;
        gc_shim_pages[p] = page != 0 && page < 0x02000000u ? m.at(page) : nullptr;
    }
}

void* Shim::resolve_handle(std::int32_t handle) const {
    const std::uint32_t base = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(gc_shim_hero));
    const std::uint32_t offset = static_cast<std::uint32_t>(handle) - base;
    return offset < kHeroBytes ? gc_shim_hero + offset : nullptr;
}

// --- The pause: Going Commando's game mode 4, mode 7, in Ratchet & Clank's freeze mode ---

// request_begin_slot(4, 1, 7, 0, 0): Going Commando asks for game mode 4 and its mode switch calls
// menu_set_mode(7, ...). Here: Ratchet & Clank's mode_freeze_init with the mod's kind (it saves the
// mode, sets mode 4, pauses sound group 0x1D and the music), then the copied menu_set_mode(7). Going
// Commando's menu_set_mode does not pause the music for mode 7, Ratchet & Clank's mode_freeze_init
// does: the music is unpaused after it.
void Shim::begin_paused(int arg) {
    game.mode_freeze_init(kFreezeKind, 0, 0);
    menu_set_mode(kModalQuickSelect, arg);
    game.music_unpause();
    m_entered = true;
    m_freeze_ticks = 0;
    m_draws_since_update = 0;
    m_record_opened = false;
}

// request_end_slot(0, 0): back to the mode the freeze was entered from (as update_mode_freeze's
// own exits do: g_game_mode = mode_freeze_info + 0x14), then Going Commando's mode switch ends the
// modal mode: the copied menu_end (the sounds and the music back on, the pad's history cleared).
// The layer's watchdog ends it the same way, and says why in the log (a normal end is not logged).
void Shim::end_paused(const char* reason) {
    // A normal modal exit precedes the wheel's final fade/commit in play.
    // Keep its EQS page until the selector itself closes; watchdogs abort it.
    if (reason != nullptr) eqs_end();
    const Memory& m = game.memory;
    const Disc& a = m_level.at;
    m.write<std::int32_t>(a.game_mode, m.read<std::int32_t>(a.mode_freeze_info + kFreezeSavedMode));
    menu_end();
    std::memset(gc_shim_modal_state + kModalPhase, 0, sizeof(std::int32_t));
    m_entered = false;
    if (reason != nullptr && game.log) {
        char line[200];
        std::snprintf(line, sizeof line, "the paused quick select ended by its watchdog: %s", reason);
        game.log(line);
    }
}

const char* Shim::paused_open_refusal() const {
    const Memory& m = game.memory;
    if (m_paused_disabled) {
        return "the paused open is off: the freeze's tick hook did not run";
    }
    if (!paused_path_available()) {
        return "the quick select's callbacks were not found in this level";
    }
    if (m.read<std::int32_t>(m_level.at.game_mode) != kModePlay) {
        return "the game mode is not play";
    }
    if (m_entered || (freeze_kind_is_ours() && m.read<std::int32_t>(m_level.at.game_mode) == kModeFreeze)) {
        return "the freeze is already on";
    }
    return nullptr;
}

// `blocked` in the copied gate (nonzero: the quick select may open): whether the paused open can be
// entered now (request_begin_slot would take it: this is checked before hero_update_button_hold_timers
// runs, so the double tap's timers run once a press, as when Going Commando's mode change stops its
// gate) and Ratchet & Clank's own conditions for opening the quick select with Triangle.
bool Shim::selector_open_allowed() const {
    if (paused_open_refusal() != nullptr) {
        return false;
    }
    const Memory& m = game.memory;
    const Disc& a = m_level.at;
    const std::uint32_t h = a.hero;
    const std::uint8_t vehicle = m.read<std::uint8_t>(h + kHeroVehicleMode);
    const bool override_locks = vehicle == 1 || m.read<std::int16_t>(h + kHeroSelectOverride) != 0;
    // hero_update_transitions (hero_item_input_and_hud.c:110-116): Triangle opens the eight-slot
    // quick select when the item slot is not locked, no timed HUD shows, the item locks allow it,
    // and he is on foot or in mode 3 (mode 1, Clank, opens Ratchet & Clank's four-slot radial,
    // which stays the game's).
    return m.read<std::uint8_t>(h + kHeroSwitchLock) != 2 && m.read<std::int32_t>(a.timed_hud_visible) == 0 &&
           (m.read<std::uint8_t>(h + kHeroUseLocked) == 0 || override_locks) &&
           (m.read<std::uint8_t>(h + kHeroSelectBlocked) == 0 || override_locks) && (vehicle == 0 || vehicle == 3);
}

std::string Shim::describe_gate() const {
    const Memory& m = game.memory;
    const Disc& a = m_level.at;
    const std::uint32_t h = a.hero;
    const int window = gc_shim_gate_window(gc_shim_press_window);
    const int within = PadPressedWithin(kTriangle, window, nullptr) != 0;
    const int hold = m.read<std::int32_t>(a.cinematic_frames);
    const int loop_mode = m.read<std::int32_t>(a.cinematic_mode);
    const std::uint8_t vehicle = m.read<std::uint8_t>(h + kHeroVehicleMode);
    const bool override_locks = vehicle == 1 || m.read<std::int16_t>(h + kHeroSelectOverride) != 0;
    const char* refusal = paused_open_refusal();
    const char* fail = nullptr;
    auto first = [&fail](bool bad, const char* what) {
        if (bad && fail == nullptr) {
            fail = what;
        }
    };
    first(!within, "Triangle not within the press window");
    first(hold < 8, "fewer than 8 ticks since the game mode changed");
    first(refusal != nullptr, refusal != nullptr ? refusal : "");
    first(m.read<std::uint8_t>(h + kHeroSwitchLock) == 2, "the item slot is locked");
    first(m.read<std::int32_t>(a.timed_hud_visible) != 0, "a timed HUD shows");
    first(m.read<std::uint8_t>(h + kHeroUseLocked) != 0 && !override_locks, "the item use is locked");
    first(m.read<std::uint8_t>(h + kHeroSelectBlocked) != 0 && !override_locks, "the item select is blocked");
    first(vehicle != 0 && vehicle != 3, "not on foot (vehicle mode)");
    first(loop_mode != 0, "the level's game mode is not 0");
    first(gc_shim_input_block != 0, "input blocked");
    first(gc_shim_options[3] == 0, "\"Pause while open\" is off");
    char line[640];
    std::snprintf(line, sizeof line,
                  "Triangle: %s%s (window %d -> %d polls, within %d; hold frames %d; level game mode %d; game mode %d "
                  "[0x%08X]; freeze kind %d; path %s; switch lock %d, timed HUD %d, use lock %d, select lock %d, "
                  "override %d, vehicle mode %d; option %d)",
                  fail == nullptr ? "the gate opens" : "the gate does not open: ", fail == nullptr ? "" : fail,
                  gc_shim_press_window, window, within, hold, loop_mode, m.read<std::int32_t>(a.game_mode), a.game_mode,
                  m.read<std::int32_t>(a.mode_freeze_info + kFreezeInfoKind), paused_path_available() ? "found" : "missing",
                  m.read<std::uint8_t>(h + kHeroSwitchLock), m.read<std::int32_t>(a.timed_hud_visible),
                  m.read<std::uint8_t>(h + kHeroUseLocked), m.read<std::uint8_t>(h + kHeroSelectBlocked),
                  m.read<std::int16_t>(h + kHeroSelectOverride), vehicle, gc_shim_options[3]);
    return line;
}

// Going Commando's swap request (hero+0x2538 = 1, level_set_flag_d_true) is Ratchet & Clank's
// own double-tap swap (hero_update_transitions, hero_item_input_and_hud.c:152-168): the quick
// swap item (or the last weapon, with the wrench in hand) pending, the usage entry 31 counted, the
// two timers cleared.
void Shim::quick_swap() {
    const Memory& m = game.memory;
    const Disc& a = m_level.at;
    Memory copy{gc_shim_hero};
    const std::int32_t swap = m.read<std::int32_t>(a.quick_swap_item);
    const std::int32_t item = m.read<std::int32_t>(a.hero + kHeroItem);
    const std::int32_t next = item == 8 ? m.read<std::int32_t>(a.last_weapon) : swap;
    if (next == 0 || item == swap) {
        return;
    }
    const std::uint32_t entry = a.usage_stats + kUsageStride * kUsageQuickSwap;
    const std::uint16_t count = m.read<std::uint16_t>(entry);
    if (count < 0xFFFF) {
        m.write<std::uint16_t>(entry, static_cast<std::uint16_t>(count + 1));
    }
    const int best = ticks(m.read<std::int32_t>(a.event_frames)) / 600;
    if (best > m.read<std::uint16_t>(entry + 2)) {
        m.write<std::uint16_t>(entry + 2, static_cast<std::uint16_t>(best));
    }
    const std::int32_t level = m.read<std::int32_t>(a.level_index);
    m.write<std::uint32_t>(entry + 4, m.read<std::uint32_t>(entry + 4) | (1u << (level & 31)) | 0x80000000u);
    copy.write<std::int32_t>(kGcHeroPending, next);
    copy.write<std::int16_t>(kGcHeroTapTimer, 0);
    copy.write<std::int16_t>(kGcHeroTapWindow, 0);
}

// hud_set_slot_contents(0x23, 0, menu_init_panel_layout, weapon_wheel_update_select,
// weapon_wheel_draw, 0, 0): Ratchet & Clank's queue_animation_update(3, 0, its selector's init,
// update and draw, 0, 0), as hero_update_transitions queues it; the hook points run the copied
// code in those three. The same callbacks again keep the open record.
void Shim::queue_selector() {
    if (!paused_path_available()) {
        return;
    }
    game.queue_animation_update(kSelectorKey, 0, m_level.selector_init, m_level.selector_update,
                                m_level.selector_draw, 0, 0);
}

// level_forward_value_b(360): the health meter shown; level_forward_value(180): the bolt counter.
// Ratchet & Clank's, as handle_pause_input shows both on Select.
void Shim::show_counters_health(int frames_shown) {
    game.hud_keep_visible(game.show_health_hud(), ticks(frames_shown));
}

void Shim::show_counters_bolts(int frames_shown) {
    if (m_level.bolt_update == 0) {
        return;
    }
    const int handle = game.queue_animation_update(2, 0x754E, m_level.bolt_init, m_level.bolt_update,
                                                   m_level.bolt_draw, m_level.at.bolts, 9999999);
    game.hud_keep_visible(handle, ticks(frames_shown));
}

// Going Commando's ring (weapon_wheel_draw's lines 44-55) is its resident HUD's labels, which
// Ratchet & Clank does not have. Its own ring in their place, as draw_hud_item_selector_panel draws
// it (games/rac1/decomp weapon_hud_and_counters.c:1303-1360, reimplemented): the four mirrored panel
// quarters (sprite 0xE934) at quick_select_width x quick_select_height, the items' icons on the ring
// (quick_select_center_x/y, ring_radius, ring_aspect; a gold weapon's frame 4), the unselected ones
// at quick_select_unselected_alpha_scale, and the marker (0xE99C) at the selection, pulsing as
// Ratchet & Clank's does (its alpha times 0.875 + 0.125 * sin over quick_select_pulse_period ticks).
// The alpha is Going Commando's (fade[0] / 8 * 128). The ring is centred on Going Commando's panel
// (its 210 x 200 from menu_init_panel_layout, at the record's place as Ratchet & Clank's
// hud_screen_position puts it; its middle, x + 105, is where Going Commando's text is centred): where
// Ratchet & Clank draws its own ring when the record's anchor centres it.
void Shim::draw_ring(std::uint8_t* menu, int x, int y, int alpha) {
    const Memory& m = game.memory;
    const Disc& a = m_level.at;
    const std::uint32_t record = guest(menu);
    const Memory rec{menu};
    const int width = m.read<std::int32_t>(a.width);
    const int height = m.read<std::int32_t>(a.height);
    const int rx = x + (rec.read<std::int32_t>(kRecordWidth) - width) / 2;
    const int ry = y + (rec.read<std::int32_t>(kRecordHeight) - height) / 2;
    const int half_w = width / 2;
    const int half_h = height / 2;
    const int panel = game.get_icon_frame(kPanelSprite, 0);
    game.draw_hud_sprite(panel, rx, ry, half_w, half_h, alpha);
    game.draw_hud_sprite(panel, rx + width, ry + height, -half_w, -half_h, alpha);
    game.draw_hud_sprite(panel, rx + width, ry, -half_w, half_h, alpha);
    game.draw_hud_sprite(panel, rx, ry + height, half_w, -half_h, alpha);

    const int count = m.read<std::int32_t>(a.slot_count);
    const int page = m.read<std::int32_t>(a.page);
    if (page < 0 || static_cast<std::size_t>(page) >= kPages || gc_shim_pages[page] == nullptr) {
        return;
    }
    const Memory entries{static_cast<std::uint8_t*>(gc_shim_pages[page])};
    const int selected = rec.read<std::int32_t>(kRecordSelected);
    const float radius = m.read<float>(a.ring_radius);
    const float aspect = m.read<float>(a.ring_aspect);
    const float unselected = m.read<float>(a.unselected_alpha);
    const int cx = m.read<std::int32_t>(a.center_x);
    const int cy = m.read<std::int32_t>(a.center_y);
    for (int i = 0; i < count && i < 8; ++i) {
        const float angle = math_add_rots((static_cast<float>(i) + static_cast<float>(i)) * kPi / static_cast<float>(count) - kPi,
                                          1.5707964f);
        const int icon_x = rx + cx + static_cast<int>(radius * math_cos(angle) * aspect);
        const int icon_y = ry + cy + static_cast<int>(radius * math_sin(angle));
        const std::int32_t icon = entries.read<std::int32_t>(static_cast<std::uint32_t>(i) * 28);
        const std::int32_t item = entries.read<std::int32_t>(static_cast<std::uint32_t>(i) * 28 + 0x18);
        if (icon == 0) {
            continue;
        }
        if (selected == i) {
            const int pulse = std::max(1, m.read<std::int32_t>(a.pulse_period));
            float phase = math_int_to_float(m.read<std::int32_t>(a.level_frame_counter) % pulse);
            phase /= math_int_to_float(pulse);
            phase *= 6.28318f;
            const float wave = math_sin(phase - 3.14159f) * 0.125f + 0.875f;
            const int marker_alpha = math_float_to_int(static_cast<float>(alpha) * wave);
            game.draw_hud_sprite(game.get_icon_frame(kMarkerSprite, 0), icon_x - 0x13, icon_y - 0x13, 0x26, 0x26,
                                 marker_alpha);
        }
        const int icon_alpha = i != selected ? math_float_to_int(static_cast<float>(alpha) * unselected) : alpha;
        const bool gold = item > 0 && item < 256 && m.read<std::uint8_t>(a.gold + static_cast<std::uint32_t>(item)) != 0;
        game.hud_frame(record, game.get_icon_frame(icon, gold ? 4 : 0), icon_x, icon_y, 1, icon_alpha);
    }
}

// --- The hook points ---

// rac1.level.update_play (handle_pause_input, game mode 0's tick): Going Commando's gate first
// (level_update_game_frame_input, cut to the quick select's paused open); when it opens the quick
// select the tick is the freeze's (Going Commando returns before its world update), else the press
// window (hero_update_hit_streak_effect, cut) and the game's tick.
void Shim::update_play(const std::function<void()>& original) {
    const Memory& m = game.memory;
    const Disc& a = m_level.at;
    ++m_play_ticks;
    const bool pressed = ((m.read<std::uint32_t>(a.pad + kPadPressed) |
                           m.read<std::uint32_t>(a.pad + kPadPressedUnmasked)) & kTriangle) != 0;
    std::string gate;
    if (pressed) {
        gate = describe_gate();
    }
    // The request's state before this tick's state machine (run once, inside the copied gate, at
    // Going Commando's place) and the masked held word it reads.
    const int request_before = gc_shim_hud_flag_b;
    const bool held = (m.read<std::uint32_t>(a.pad + kPadHeld) & kTriangle) != 0;
    const bool swap_flag = gc_shim_hud_flag_a != 0;
    int took = 0;
    {
        CopyScope scope(*this);
        took = level_update_game_frame_input();
    }
    // One line a press: the request's (trace_request) when the press asks for the quick select, else
    // why the gate did not.
    if (pressed && request_before == 0 && gc_shim_hud_flag_b == 0 && game.info) {
        game.info(gate.c_str());
    }
    trace_request(request_before, held, swap_flag);
    if (took != 0) {
        return;
    }
    hero_update_hit_streak_effect();
    TapSnapshot before;
    before.tap = m.read<std::int16_t>(a.hero + kHeroTapTimer);
    before.window = m.read<std::int16_t>(a.hero + kHeroTapWindow);
    before.item = m.read<std::int32_t>(a.hero + kHeroItem);
    before.pending = m.read<std::int32_t>(a.hero + kHeroPendingItem);
    before.swap_item = m.read<std::int32_t>(a.quick_swap_item);
    before.last_weapon = m.read<std::int32_t>(a.last_weapon);
    original();
    explain_double_tap(before);
}

// The HUD-open request in the log, one line a request: the tick it was asked at, then each tick's
// state before and after (the state machine, then the gate: "21" a cancel the gate asked again, "23"
// done) with the masked held word's Triangle bit (H held, - not), and how it ended.
void Shim::trace_request(int before, bool held, bool swap_flag) {
    const int after = gc_shim_hud_flag_b;
    if (before == 0 && after == 0) {
        return;
    }
    if (before == 0) {
        m_request_tick = m_play_ticks;
        m_request_trace.clear();
    }
    char step[32];
    std::snprintf(step, sizeof step, " %d%d%c(%d,%d)", before, after, held ? 'H' : '-',
                  game.memory.read<std::int16_t>(m_level.at.hero + kHeroTapTimer),
                  game.memory.read<std::int16_t>(m_level.at.hero + kHeroTapWindow));
    if (m_request_trace.size() < 900) {
        m_request_trace += step;
    }
    const char* end = nullptr;
    if (after == 3) {
        end = "done: the paused open";
    } else if (after == 0) {
        end = swap_flag ? "cancelled: a double tap's swap" : !held ? "cancelled: Triangle let go" : "cancelled";
    }
    if (end == nullptr) {
        return;
    }
    char line[1100];
    std::snprintf(line, sizeof line,
                  "HUD-open request asked at play tick %d, %s after %d ticks (state before/after, Triangle held, "
                  "tap timer, window):%s",
                  m_request_tick, end, m_play_ticks - m_request_tick, m_request_trace.c_str());
    if (game.info) {
        game.info(line);
    }
    m_request_trace.clear();
}

// Ratchet & Clank's own double tap (hero_update_transitions, hero_item_input_and_hud.c:142-170: the
// same timers as Going Commando's hero_update_button_hold_timers, which Going Commando's hero decider
// runs in its place), seen from its effect on a tick Triangle is let go: both timers count down,
// then a release with the window open swaps: pending_item[0] = the quick swap item (or, with the
// wrench in hand, last_weapon_id[0]) and both timers cleared, the only place that clears both. The
// hero's update applies the pending item the same tick (update_hero_equipped_items, after
// hero_update_transitions: hero_switch_item), so the swap is seen by the cleared timers, not the
// pending item. Its swap is Going Commando's swap request, whose flag (level_set_flag_d_true)
// cancels the HUD-open request. In the log: each swap, and each release in the window that does
// not swap, and why.
//
// Ratchet & Clank's words, as hero_switch_item keeps them (hero_animation_slots.c:1796-2000): on a
// change of slot 0's item to `u` (not item 0x24, the wrench not in hand), the quick swap item
// (quick_swap_item, D_0015EE8C, g_game_language[1] in the decomp) takes last_weapon_id[0], then
// last_weapon_id[0] takes `u`. So last_weapon_id[0] is the item last switched to (the one in hand),
// the quick swap item the one before it: the double tap swaps to the quick swap item, and uses
// last_weapon_id[0] only with the wrench in hand (the weapon before the wrench).
void Shim::explain_double_tap(const TapSnapshot& b) {
    const Memory& m = game.memory;
    const Disc& a = m_level.at;
    if ((m.read<std::uint32_t>(a.pad + kPadRawReleased) & kTriangle) == 0) {
        return;
    }
    const std::int16_t tap = m.read<std::int16_t>(a.hero + kHeroTapTimer);
    const std::int16_t window = m.read<std::int16_t>(a.hero + kHeroTapWindow);
    const std::int16_t tap_counted = b.tap != 0 ? static_cast<std::int16_t>(b.tap - 1) : 0;
    const std::int16_t window_counted = b.window != 0 ? static_cast<std::int16_t>(b.window - 1) : 0;
    const std::int32_t next = b.item == 8 ? b.last_weapon : b.swap_item;
    char line[320];
    line[0] = 0;
    if (window_counted != 0 && tap == 0 && window == 0) {
        gc_shim_hud_flag_a = 1;
        std::snprintf(line, sizeof line,
                      "double tap: swap from item 0x%X to 0x%X (%s; tap timer %d, window %d before); now pending 0x%X, "
                      "the hand's next item 0x%X, quick swap 0x%X, last weapon 0x%X",
                      b.item, next, b.item == 8 ? "the wrench: the last weapon" : "the quick swap item", b.tap, b.window,
                      m.read<std::int32_t>(a.hero + kHeroPendingItem), m.read<std::int32_t>(a.hero + kHeroRequestedItem),
                      m.read<std::int32_t>(a.quick_swap_item), m.read<std::int32_t>(a.last_weapon));
    } else if (window_counted != 0) {
        const char* why = next == 0 ? (b.item == 8 ? "no last weapon" : "no quick swap item")
                          : b.item == b.swap_item ? "the quick swap item is in hand"
                          : (tap == tap_counted && window == window_counted)
                              ? "the hero's update did not reach its double tap (hurt, an item's input, water or firing first)"
                              : "the game's rule refused it";
        std::snprintf(line, sizeof line,
                      "double tap: a second release in the window, no swap: %s (item 0x%X, quick swap 0x%X, last weapon "
                      "0x%X; tap timer %d -> %d, window %d -> %d)",
                      why, b.item, b.swap_item, b.last_weapon, b.tap, tap, b.window, window);
    }
    if (line[0] != 0 && game.info) {
        game.info(line);
    }
}

// rac1.freeze.update (update_mode_freeze): the mod's kind runs Going Commando's UpdateModalState
// (its prologue and case 7), then the game's (sound_update once, the two countdowns, and the sounds
// and the music back on once the mode was left).
//
// Going Commando's phase 1 ends when g_modal_timer reaches 0; menu_set_state_word_three sets it to
// 3, in phase 0 and in the quick select's update each frame the quick select is active, so phase 1
// lasts as long as the quick select (its commit moves it on: menu_advance_state_in_mode_seven).
//
// Every way out ends the freeze: a freeze of the mod's kind that is not the paused quick select (or
// whose callbacks are gone) is ended at once, and one whose wheel never opened is ended after half
// a second (the watchdog).
void Shim::freeze_update(const std::function<void()>& original) {
    const Memory& m = game.memory;
    if (!freeze_kind_is_ours() || m.read<std::int32_t>(m_level.at.game_mode) != kModeFreeze) {
        original();
        return;
    }
    m_draws_since_update = 0;
    if (!in_wheel_freeze() || !m_entered) {
        end_paused("a freeze of the mod's kind without the paused quick select");
        original();
        return;
    }
    if (!paused_path_available()) {
        end_paused("the quick select's callbacks are missing");
        original();
        return;
    }
    ++m_freeze_ticks;
    {
        CopyScope scope(*this);
        UpdateModalState();
    }
    if (m_entered && in_wheel_freeze() && !m_record_opened && m_freeze_ticks > frames(30)) {
        end_paused("the wheel's HUD record never opened");
    }
    original();
}

// rac1.freeze.draw (draw_dialog_background_and_text): the mod's kind draws Going Commando's
// picture (gfx_render_frame_extra_layers: the world and its HUD again, no dialog), the tick it
// ended in included. Two pictures with no tick of the freeze between them: the freeze's tick hook
// does not run (a port that bypasses it), so the freeze is ended here and the paused open is off
// for the session (the quick select then opens over the running game).
void Shim::freeze_draw(const std::function<void()>& original) {
    const Memory& m = game.memory;
    if (!freeze_kind_is_ours()) {
        original();
        return;
    }
    if (m_entered && m.read<std::int32_t>(m_level.at.game_mode) == kModeFreeze && ++m_draws_since_update >= 2) {
        m_paused_disabled = true;
        log_once("hook bypassed", "rac1.freeze.update did not run in the freeze: the paused quick select is off "
                                  "for this session (it opens over the running game)", true);
        end_paused("rac1.freeze.update did not run");
    }
    gfx_render_frame_extra_layers();
}

// The provider returns native item IDs only. Use host-side copies of the eight
// entries and assignment array, never change canonical game/save memory.
namespace {
class ScopedEqsPage {
    void* saved_entries = nullptr;
    void* saved_items = nullptr;
    std::array<std::array<std::int32_t,7>,8> entries{};
    std::array<std::int32_t,8> items{};
    bool active = false;
public:
    ScopedEqsPage(const gcw::gc::Game& game, const gcw::Disc& a, const eqs_wheel_page_v2& page) {
        if (page.page_index == 0 || page.page_index >= page.page_count || page.page_count > 8 ||
            !gc_shim_pages[0] || !gc_shim_slot_items ||
            game.memory.read<std::int32_t>(a.page) != 0 ||
            game.memory.read<std::int32_t>(a.slot_count) != 8) return;
        for (int id : page.native_ids) if (id < 0 || id > 39) return;
        std::memcpy(entries.data(), gc_shim_pages[0], sizeof entries);
        for (std::size_t i = 0; i < items.size(); ++i) {
            const auto id = page.native_ids[i];
            items[i] = id;
            entries[i][6] = id;
            entries[i][0] = id == 0 ? 0 : game.memory.read<std::uint16_t>(
                a.item_defs + static_cast<std::uint32_t>(id) * gcw::kItemDefStride + gcw::kItemDefIcon);
        }
        saved_entries = gc_shim_pages[0];
        saved_items = gc_shim_slot_items;
        gc_shim_pages[0] = entries.data();
        gc_shim_slot_items = items.data();
        active = true;
    }
    ~ScopedEqsPage() {
        if (active) { gc_shim_pages[0] = saved_entries; gc_shim_slot_items = saved_items; }
    }
    ScopedEqsPage(const ScopedEqsPage&) = delete;
    ScopedEqsPage& operator=(const ScopedEqsPage&) = delete;
    bool applied() const { return active; }
};
}

void Shim::eqs_begin(std::uint32_t record) {
    eqs_end();
    if (!game.eqs_page) return;
    eqs_wheel_page_v2 page{};
    page.size = sizeof(page);
    const auto held = game.memory.read<std::uint32_t>(m_level.at.pad + kPadHeldUnmasked);
    if (game.eqs_page(EQS_WHEEL_BEGIN, held, &page)) {
        m_eqs_active = true;
        m_eqs_record = record;
        log_once("eqs bridge", "Expanded Quick Select page provider connected");
    }
}
void Shim::eqs_end() {
    if (m_eqs_active && game.eqs_page) game.eqs_page(EQS_WHEEL_END, 0, nullptr);
    m_eqs_active = false;
    m_eqs_record = 0;
}
bool Shim::eqs_snapshot(uint32_t event, std::uint32_t record, eqs_wheel_page_v2& page) {
    if (!m_eqs_active || m_eqs_record != record || !game.eqs_page) return false;
    page.size = sizeof(page);
    const auto held = game.memory.read<std::uint32_t>(m_level.at.pad + kPadHeldUnmasked);
    return game.eqs_page(event, held, &page) && page.page_count >= 1 &&
           page.page_count <= 8 && page.page_index < page.page_count;
}

// rac1.hud.quick_select.init: Going Commando's menu_init_panel_layout (with weapon_menu_init_slots).
// With "Pause while open" on, a record Ratchet & Clank's own Triangle opened (not the mod's freeze)
// is refused, as Going Commando's decider opens the HUD quick select only with the option off
// (L00_002CB3A8-decider.c:185, g_option_toggle_b[1]): the quick select is the paused one, which a
// held Triangle opens.
void Shim::selector_init(std::uint32_t record) {
    const Memory& m = game.memory;
    m_refused.erase(std::remove(m_refused.begin(), m_refused.end(), record), m_refused.end());
    rebuild_pages();
    fill_weapon_table();
    menu_init_panel_layout(m.at(record));
    rebuild_pages();
    fill_weapon_table();
    if (in_wheel_freeze()) {
        m_record_opened = true;
        eqs_begin(record);
        log_once("record in freeze", "the paused quick select's HUD record opened");
        return;
    }
    if (!pause()) {
        eqs_begin(record);
        log_once("record hud", "the game's Triangle opened the quick select over the running game (\"Pause while open\" off)");
        return;
    }
    m_refused.push_back(record);
    log_once("record refused", "the game's own quick select refused (\"Pause while open\" on: the paused one opens on a held Triangle)");
}

// rac1.hud.quick_select.update: Going Commando's weapon_wheel_update_select (a refused record is
// closed at once, as the quick select closes itself).
void Shim::selector_update(std::uint32_t record) {
    const Memory& m = game.memory;
    if (std::find(m_refused.begin(), m_refused.end(), record) != m_refused.end()) {
        m_refused.erase(std::remove(m_refused.begin(), m_refused.end(), record), m_refused.end());
        game.remove_hud_item(m.read<std::int32_t>(record + kRecordHandle));
        m.write<std::int32_t>(record + kRecordSlide, -6);
        return;
    }
    CopyScope scope(*this);
    rebuild_pages();
    eqs_wheel_page_v2 page{};
    if (!eqs_snapshot(EQS_WHEEL_UPDATE, record, page)) page = {};
    {
        ScopedEqsPage projection(game, m_level.at, page);
        fill_weapon_table();
        weapon_wheel_update_select(m.at(record));
    }
    // Keep the page through the entire release/fade/commit lifecycle.
    if (m_eqs_record == record && m.read<std::int32_t>(record + kRecordSlide) == -6) eqs_end();
}

// rac1.hud.quick_select.draw: Going Commando's weapon_wheel_draw.
int Shim::selector_draw(std::uint32_t record) {
    const Memory& m = game.memory;
    if (std::find(m_refused.begin(), m_refused.end(), record) != m_refused.end()) {
        return m.read<std::int32_t>(record + kRecordWidth);
    }
    rebuild_pages();
    eqs_wheel_page_v2 page{};
    const bool supplied = eqs_snapshot(EQS_WHEEL_DRAW, record, page);
    if (!supplied) page = {};
    ScopedEqsPage projection(game, m_level.at, page);
    fill_weapon_table();
    const int result = weapon_wheel_draw(reinterpret_cast<char*>(m.at(record)));
    if (supplied && page.page_count > 1 && (page.page_index == 0 || projection.applied()) &&
        game.font_print_center && game.hud_screen_position) {
        int x = m.read<std::int32_t>(record + 0x50);
        int y = m.read<std::int32_t>(record + 0x54);
        game.hud_screen_position(record, x, y);
        char label[40];
        std::snprintf(label, sizeof label, "L1  PAGE %u / %u  R1",
                      page.page_index + 1, page.page_count);
        const auto fade = std::min<unsigned>(8, m.read<std::uint8_t>(record + kRecordFades));
        game.font_print_center(x + 105, y + 190, (fade * 16u << 24) | 0x00f0f0f0u,
                               label, -1);
    }
    return result;
}

// --- Math: Ratchet & Clank's own by name, or a faithful stand-in ---

float Shim::math_sqrt(float v) const { return game.square_root ? game.square_root(v) : std::sqrt(v); }

float Shim::math_atan(float x, float y) const {
    // fast_arc_tan(x, y) (Ratchet & Clank's update: ang = fast_atan2(x, y)), the angle of (x, y)
    // from the x axis.
    return game.fast_arc_tan ? game.fast_arc_tan(x, y) : std::atan2(y, x);
}

float Shim::math_normalize(float a) const {
    if (game.normalize_angle) {
        return game.normalize_angle(a);
    }
    // FastNormalizeAngle: into -pi..pi.
    while (a > kPi) {
        a -= 2.0f * kPi;
    }
    while (a < -kPi) {
        a += 2.0f * kPi;
    }
    return a;
}

float Shim::math_sub_rots(float a, float b) const {
    return game.sub_rots ? game.sub_rots(a, b) : math_normalize(a - b);
}

float Shim::math_add_rots(float a, float b) const {
    return game.add_rots ? game.add_rots(a, b) : math_normalize(a + b);
}

float Shim::math_diff_rots(float a, float b) const {
    // FastDiffRots: how far apart two angles are, 0..pi.
    return game.diff_rots ? game.diff_rots(a, b) : std::fabs(math_normalize(a - b));
}

float Shim::math_sin(float a) const { return game.sin ? game.sin(a) : std::sin(a); }
float Shim::math_cos(float a) const { return game.cos ? game.cos(a) : std::cos(a); }
float Shim::math_int_to_float(int v) const { return game.int_to_float ? game.int_to_float(v) : static_cast<float>(v); }
int Shim::math_float_to_int(float v) const { return game.float_to_int ? game.float_to_int(v) : static_cast<int>(v); }

std::uint32_t Shim::tween_colour(std::uint32_t from, std::uint32_t to, float t) const {
    if (game.fast_tween_color) {
        return game.fast_tween_color(from, to, t);
    }
    // FastTweenColor: each byte from `from` toward `to` by t.
    std::uint32_t out = 0;
    for (int s = 0; s < 32; s += 8) {
        const float a = static_cast<float>((from >> s) & 0xFFu);
        const float b = static_cast<float>((to >> s) & 0xFFu);
        out |= (static_cast<std::uint32_t>(a + (b - a) * t) & 0xFFu) << s;
    }
    return out;
}

}  // namespace gc_weapon_wheel::gc

// --- The polyfills: Going Commando's functions the copied code calls ---

extern "C" {

// The lines the mod adds to the copied files (gc/shim/common.h).
int gc_shim_frames(int frames) { return shim().frames(frames); }

int gc_shim_gate_window(int frames) {
    return std::min(30, std::max(shim().frames(frames), gc_shim_open_delay_start + 2));
}

void gc_shim_item_0x18(void) {
    mem().write<std::uint8_t>(shim().level().at.hero + gcw::kHeroItem18, 1);
}

void gc_shim_draw_ring(char* menu, int x, int y, int alpha) {
    shim().draw_ring(reinterpret_cast<std::uint8_t*>(menu), x, y, alpha);
}

int gc_shim_selector_open_allowed(void) { return shim().selector_open_allowed() ? 1 : 0; }

int gc_shim_skip_frame(void) { return mem().read<std::int32_t>(shim().level().at.skip_frame); }

// FastDecTimer (Ratchet & Clank's and Going Commando's): 1 when the s16 timer is 0, else it counts
// down one and gives 2 when it reached 0, 0 while it runs (the port's translation of FUN_001f9770).
int gc_shim_FastDecTimer(void* timer) {
    std::int16_t v = 0;
    std::memcpy(&v, timer, sizeof v);
    if (v == 0) {
        return 1;
    }
    int n = v < 1 ? 1 : v;
    n -= 1;
    v = static_cast<std::int16_t>(n);
    std::memcpy(timer, &v, sizeof v);
    return n > 0 ? 0 : 2;
}

// The same for an address hero_update_button_hold_timers passes as an s32: in the hero's copy.
std::int32_t gc_shim_h_FastDecTimer(std::int32_t handle) {
    void* p = shim().resolve_handle(handle);
    return p != nullptr ? gc_shim_FastDecTimer(p) : 1;
}

float gc_shim_square_root(float v) { return shim().math_sqrt(v); }
// mat_fast_helper_a(x, y): the stick's angle; Ratchet & Clank's fast_arc_tan.
float gc_shim_mat_fast_helper_a(float x, float y) { return shim().math_atan(x, y); }
float gc_shim_IntToFloat(int v) { return shim().math_int_to_float(v); }
int gc_shim_FloatToInt(float v) { return shim().math_float_to_int(v); }
// fast_subtract_rotations: Ratchet & Clank's FastSubRots.
float gc_shim_fast_subtract_rotations(float a, float b) { return shim().math_sub_rots(a, b); }
float gc_shim_FastDiffRots(float a, float b) { return shim().math_diff_rots(a, b); }
// math_fast_float_g: the angle into -pi..pi, Ratchet & Clank's FastNormalizeAngle (its update writes
// quick_select_slot_angle_offset with it where Going Commando calls this).
float gc_shim_math_fast_float_g(float a) { return shim().math_normalize(a); }
unsigned int gc_shim_FastTweenColor(unsigned int from, unsigned int to, float t) {
    return shim().tween_colour(from, to, t);
}

// Going Commando's debug print and disc-read break: nothing.
void gc_shim_STUB_printf(void*) {}
void gc_shim_load_request_disc_read_break(void) {}
// Going Commando preloads the selected weapon's moby class before the commit
// (load_is_class_cached / LoadClassIntoFreeCacheSlot); Ratchet & Clank keeps its items loaded.
int gc_shim_load_is_class_cached(int) { return 1; }
void gc_shim_LoadClassIntoFreeCacheSlot(int) {}

// snd_play_from_global_bank(3, 0, 0), the click as the selection changes: Ratchet & Clank's global
// bank has no UI click, so the level bank's entry 0 (allocate_voice_for_bank_entry(0, 1, 0), the
// sound help.c plays as a help message opens), when "Click sound" is on. Other sounds: none.
int gc_shim_snd_play_from_global_bank(int sound, int, void*) {
    if (sound == 3 && shim().click() && shim().game.play_click) {
        shim().game.play_click();
    }
    return 0;
}

// hud_close_element_by_key: Ratchet & Clank's remove_hud_item.
int gc_shim_hud_close_element_by_key(int handle) { return shim().game.remove_hud_item(handle); }

// menu_set_screen_value_restart_six_fades (its resident HUD's fades): nothing of Ratchet & Clank's.
void gc_shim_menu_set_screen_value_restart_six_fades(void*, int) {}
// stub_empty_20 (func_0026F7B0): empty in the executable, which rac2-decomp's UpdateModalState is. The
// game runs a level program's copy of UpdateModalState, which calls hero_update_button_hold_timers in
// its place: USA v2.00's level 0 (UpdateModalState at 0x002F2AF0) calls 0x002CB4C0
// (func_L00_002CB218, hero_update_button_hold_timers) at 0x002F3ADC, 0x002F3B14, 0x002F3BE8 and
// 0x002F3CA4, mode 7's four phases, where the C calls stub_empty_20. So the double tap's timers run
// while the quick select holds the world: a release, then a press and release, in the pause swap.
void gc_shim_stub_empty_20(void) { hero_update_button_hold_timers(); }
// UpdateSoundVoices: Ratchet & Clank's sound_update, which update_mode_freeze runs right after.
int gc_shim_UpdateSoundVoices(void) { return 0; }

// hud_screen_position: Ratchet & Clank's (func_L00_00236400), on the record's game address: the
// record's place (+0x50, +0x54: Ratchet & Clank's for its quick select) anchored by its flags and
// size (Going Commando's 210 x 200). Going Commando's own place is its HUD's disc data, which the code
// does not give: Ratchet & Clank's is kept.
int gc_shim_hud_screen_position(void* record, int* x, int* y) {
    shim().game.hud_screen_position(guest(record), *x, *y);
    return 0;
}

// sprintf into the draw's 80-byte buffer (the format is Ratchet & Clank's hud_ammo_format).
int gc_shim_sprintf(char* out, const char* format, ...) {
    va_list args;
    va_start(args, format);
    const int n = std::vsnprintf(out, 80, format, args);
    va_end(args);
    return n;
}

unsigned int gc_shim_strlen(const char* text) { return static_cast<unsigned int>(std::strlen(text)); }

// GetLocalizedString: Ratchet & Clank's msg_string (a game address of the text).
char* gc_shim_GetLocalizedString(int id) {
    const std::uint32_t text = shim().game.msg_string(id);
    return text != 0 ? reinterpret_cast<char*>(host(text)) : nullptr;
}

// text_draw_centered_small(x, y, colour, text, length): Ratchet & Clank's FontPrintCenter.
int gc_shim_text_draw_centered_small(int x, int y, unsigned int colour, char* text, int length) {
    shim().game.font_print_center(x, y, colour, text, length);
    return 0;
}

// request_begin_slot(4, 1, 7, ...): the paused quick select (Shim::begin_paused); anything else is
// refused (-1), as Going Commando refuses a request it cannot take.
int gc_shim_request_begin_slot(int index, int, int arg_a, int arg_b, void*) {
    Shim& s = shim();
    if (index != gcw::gc::kModeFreeze || arg_a != gcw::gc::kModalQuickSelect || s.paused_open_refusal() != nullptr) {
        return -1;
    }
    s.begin_paused(arg_b);
    return 0;
}

int gc_shim_request_end_slot(int, int) {
    shim().end_paused(nullptr);
    return 0;
}

// request_is_current_slot: whether the game mode is that one.
int gc_shim_request_is_current_slot(int value) {
    return mem().read<std::int32_t>(shim().level().at.game_mode) == value ? 1 : 0;
}

// mode_get_stack_top: the mode under game mode 4 (mode_freeze_info + 0x14).
std::int32_t gc_shim_mode_get_stack_top(void) {
    return mem().read<std::int32_t>(shim().level().at.mode_freeze_info + gcw::kFreezeSavedMode);
}

// snd_send_cmd22(0x5D) / snd_send_cmd23(0x5D): pause / continue the sounds of Going Commando's group
// 0x5D: Ratchet & Clank's snd_PauseAllSoundsInGroup / snd_ContinueAllSoundsInGroup(0x1D), the group
// mode_freeze_init and update_mode_freeze pause and continue.
void gc_shim_snd_send_cmd22(std::int32_t group) {
    if (group == 0x5D) {
        shim().game.snd_pause_group(0x1D);
    }
}
void gc_shim_snd_send_cmd23(int group) {
    if (group == 0x5D) {
        shim().game.snd_continue_group(0x1D);
    }
}
void gc_shim_music_Pause(std::int32_t how) { shim().game.music_pause(how); }
void gc_shim_music_Unpause(void) { shim().game.music_unpause(); }
// snd_Pump: Ratchet & Clank's snd_FlushSoundCommands.
int gc_shim_snd_Pump(void) {
    shim().game.snd_flush();
    return 0;
}

// pad_clear_port_state: the pad's history head and count (+0x18E, +0x190), as Going Commando's
// (002B7E28.c:304), the same fields of Ratchet & Clank's PadState (hist_idx, hist_count).
void gc_shim_pad_clear_port_state(void) {
    const std::uint32_t pad = shim().level().at.pad;
    mem().write<std::int16_t>(pad + gcw::kPadHistIdx, 0);
    mem().write<std::int32_t>(pad + gcw::kPadHistCount, 0);
}

// level_set_flag_d_true, with hero+0x2538 = 1: the quick swap (Shim::quick_swap), and Going
// Commando's flag (the swap flag word, hud_clear_flag_a's), which cancels the HUD-open request.
void gc_shim_level_set_flag_d_true(void) {
    shim().quick_swap();
    gc_shim_hud_flag_a = 1;
}
// level_get_flag_d: that flag.
int gc_shim_level_get_flag_d(void) { return gc_shim_hud_flag_a; }

// hud_set_slot_contents(0x23, ...): the quick select queued (Shim::queue_selector). The radial
// (hero mode 1: weapon_wheel_init, UpdateRadialMenuSelection, DrawRadialSelectorPanel) never
// comes here: the gate opens only on foot or in mode 3.
int gc_shim_hud_set_slot_contents(int key, int, void* init, void*, void*, int, int) {
    if (key == gcw::gc::kGcSelectorKey && init == reinterpret_cast<void*>(&menu_init_panel_layout)) {
        shim().queue_selector();
    }
    return 0;
}
void gc_shim_weapon_wheel_init(unsigned char*) {}
void gc_shim_UpdateRadialMenuSelection(void) {}
int gc_shim_DrawRadialSelectorPanel(void) { return 0; }

// level_forward_value_b / level_forward_value: the health meter and the bolt counter shown.
void gc_shim_level_forward_value_b(int frames) { shim().show_counters_health(frames); }
void gc_shim_level_forward_value(int frames) { shim().show_counters_bolts(frames); }

// hud_update_record_timers(1): Ratchet & Clank's update_hud_elements(1), the same 13 records' update
// (their callbacks, slides and timeouts), which handle_pause_input runs and the freeze mode does not.
int gc_shim_hud_update_record_timers(int what) { return shim().game.update_hud_elements(what); }
// level_run_callbacks, after it in both games' play tick: Ratchet & Clank's Help_Update.
void gc_shim_level_run_callbacks(void) {
    if (shim().game.help_update) {
        shim().game.help_update();
    }
}

// gfx_append_draw_list_buffer_tag(0) and RenderFrame: Ratchet & Clank's append_gif_transfer_packet
// and DrawDebugProfiler, as draw_dialog_background_and_text draws the world (and its HUD) under the
// freeze; the draw mask (g_render_layer_flags, 0x100FF) is Ratchet & Clank's debug_profiler_color.
void gc_shim_gfx_append_draw_list_buffer_tag(int) { shim().game.append_gif_transfer_packet(); }
void gc_shim_RenderFrame(void) { shim().game.draw_debug_profiler(); }
// menu_draw_pause_panels: not reached (the world is drawn under mode 7: it is entered from play).
void gc_shim_menu_draw_pause_panels(int) {}
// menu_draw_message_panel: draws nothing while the modal state's camera flag is set, which case 7
// sets first; no freeze dialog (draw_freeze_message_box) here.
void gc_shim_menu_draw_message_panel(void) {}

}  // extern "C"
