// SPDX-License-Identifier: GPL-3.0-or-later
//
// The translation layer: Going Commando's weapon quick select (src/gc/*.c, copied from
// rac2-decomp) run on Ratchet & Clank's memory, functions and hook points.
//
// Ratchet & Clank's quick select is Going Commando's ancestor, record for record: the same 0x90-byte
// HUD record (its update and draw callbacks take it), the same pad, the same page of seven-word slot
// entries. The copied functions take the record as a host pointer (game memory's base plus its
// address) and read the pad and the quick select's words in game memory through pointers
// (gc/shim/common.h). What Ratchet & Clank keeps at other offsets is a copy (the hero, the usage
// statistics, the weapon table) that the layer fills before the copied code runs and writes back
// after. Going Commando's functions are polyfills (rac1_shim.cpp): each one is Ratchet & Clank's
// own function for the job, found by name, or a stand-in, or nothing; its comment says which.
//
// The pause is Going Commando's own: its game mode 4 with modal mode 7 (UpdateModalState's case 7,
// gfx_render_frame_extra_layers), run in Ratchet & Clank's game mode 4, the freeze mode
// (update_mode_freeze, draw_dialog_background_and_text), with a freeze kind of the mod's own (9:
// update_mode_freeze has cases 0 to 8 only, so 9 does nothing there but the sound and two
// countdowns). Going Commando's gate (level_update_game_frame_input) runs before the level's tick
// (rac1.level.update_play) and, when it opens the quick select, the tick is skipped, as Going
// Commando returns before its world update.
//
// Time: Going Commando counts 60 frames a second; Ratchet & Clank's tick is a 50th on PAL. The
// copied lines that count frames are marked and go through gc_shim_frames ((n * 5 + 2) / 6 on PAL,
// as Going Commando's PAL build scales its press window); the layer's own counts of Ratchet & Clank's
// go through ticks() (scale_ticks: n * frame_scale + 0.5).
//
// Nothing here includes the SDK: mod.cpp fills Game from it, and the tests from a fake.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

#include "../layout.hpp"

namespace gc_weapon_wheel::gc {

// Game memory: game address a is base + a.
struct Memory {
    std::uint8_t* base = nullptr;

    template <class T>
    T read(std::uint32_t address) const {
        T v;
        std::memcpy(&v, base + address, sizeof v);
        return v;
    }
    template <class T>
    void write(std::uint32_t address, const T& v) const {
        std::memcpy(base + address, &v, sizeof v);
    }
    std::uint8_t* at(std::uint32_t address) const { return base + address; }
    std::uint32_t address_of(const void* host) const {
        return static_cast<std::uint32_t>(static_cast<const std::uint8_t*>(host) - base);
    }
};

// The mod's freeze kind: one update_mode_freeze has no case for.
constexpr std::int32_t kFreezeKind = 9;
// Going Commando's modal mode of the paused quick select.
constexpr std::int32_t kModalQuickSelect = 7;
// Ratchet & Clank's game modes: play and the freeze mode.
constexpr std::int32_t kModePlay = 0;
constexpr std::int32_t kModeFreeze = 4;
// The quick select's HUD key (queue_animation_update's first argument: slot 3) and Going
// Commando's (hud_set_slot_contents' 0x23).
constexpr std::int32_t kSelectorKey = 3;
constexpr std::int32_t kGcSelectorKey = 0x23;
// The HUD-open request's countdown start in Going Commando's data (ref_L00_002FC058_01: 4 frames in
// every USA v2.00 level program, 0x001A9374 in level 0's), and the setting's longer choices.
constexpr int kGcOpenDelay = 4;
constexpr int kOpenDelayLittleLonger = 8;
constexpr int kOpenDelayLonger = 12;


// Sizes of the layer's copies (Going Commando's offsets).
constexpr std::size_t kHeroBytes = 0x2540;
constexpr std::size_t kStatsBytes = 0x100;
constexpr std::size_t kWeaponRecord = 0xE0;
constexpr std::size_t kWeapons = 64;
constexpr std::size_t kModalBytes = 0x100;
constexpr std::size_t kPages = 8;

// Going Commando's hero, at its offsets (the copy's fields).
constexpr std::uint32_t kGcHeroItem = 0x1248;
constexpr std::uint32_t kGcHeroState = 0x2294;
constexpr std::uint32_t kGcHeroMode = 0x22B4;
constexpr std::uint32_t kGcHeroUseLocked = 0x22BC;
constexpr std::uint32_t kGcHeroPending = 0x22C8;
constexpr std::uint32_t kGcHeroTapTimer = 0x24F8;
constexpr std::uint32_t kGcHeroTapWindow = 0x24FA;
constexpr std::uint32_t kGcHeroSwapRequest = 0x2538;
// Going Commando's usage statistics' quick select entry (count u16, frame u32, levels u32).
constexpr std::uint32_t kGcStatsQuickSelect = 0xF0;
// Going Commando's modal state (UpdateModalState's ModalState).
constexpr std::uint32_t kModalMode = 0x40;
constexpr std::uint32_t kModalPhase = 0x58;
constexpr std::uint32_t kModalCameraActive = 0x7E;
constexpr std::uint32_t kModalWorldDrawn = 0x88;

// What the layer calls in the game. mod.cpp fills it from the SDK (the game's functions by name,
// its stack for pointers to locals); the tests from a fake. The math entries are optional: without
// one the layer uses a faithful C stand-in.
struct Game {
    Memory memory;
    Disc disc{};
    // Level 0's data address in the level loaded (Mod::relocate_level(0, a)).
    std::function<std::uint32_t(std::uint32_t)> relocate;
    // The executable's data address for the program loaded (Mod::relocate(a)).
    std::function<std::uint32_t(std::uint32_t)> relocate_exe;
    // Whether the function at code address `address` of the level loaded is the one of that name
    // (Mod::function_at and Mod::function).
    std::function<bool(std::uint32_t address, const char* name)> code_matches;

    std::function<void(int, int, int)> mode_freeze_init;
    std::function<int(int)> update_hud_elements;
    std::function<void()> help_update;
    std::function<void()> music_unpause;
    std::function<void(int)> music_pause;
    std::function<void(int)> snd_pause_group;
    std::function<void(int)> snd_continue_group;
    std::function<void()> snd_flush;
    std::function<void(int, int, int, int, int, int)> draw_hud_sprite;
    std::function<int(int, int)> get_icon_frame;
    std::function<void(std::uint32_t, int, int, int, std::uint32_t, int)> hud_frame;
    // FontPrintCenter(x, y, colour, text, length): mod.cpp puts the text on the game's stack.
    std::function<void(int, int, std::uint32_t, const char*, int)> font_print_center;
    std::function<std::uint32_t(int)> msg_string;
    std::function<std::uint32_t(std::uint32_t, std::uint32_t, float)> fast_tween_color;
    // hud_screen_position(record, &x, &y): mod.cpp passes x and y on the game's stack.
    std::function<void(std::uint32_t, int&, int&)> hud_screen_position;
    std::function<int()> show_health_hud;
    std::function<int(int, int)> hud_keep_visible;
    std::function<int(int, int, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, int)>
        queue_animation_update;
    std::function<int(int)> remove_hud_item;
    std::function<void()> append_gif_transfer_packet;
    std::function<void()> draw_debug_profiler;
    // The click: allocate_voice_for_bank_entry(0, 1, 0), the level bank's entry 0.
    std::function<void()> play_click;

    std::function<float(float)> square_root;
    std::function<float(float, float)> fast_arc_tan;
    std::function<float(float, float)> diff_rots;
    std::function<float(float, float)> sub_rots;
    std::function<float(float)> normalize_angle;
    std::function<float(float, float)> add_rots;
    std::function<float(float)> sin;
    std::function<float(float)> cos;
    std::function<float(int)> int_to_float;
    std::function<int(float)> float_to_int;

    // The session log: a warning, and a line of information.
    std::function<void(const char*)> log;
    std::function<void(const char*)> info;
};

// Level 0's data and code addresses in the level loaded.
struct LevelAddresses {
    Disc at{};
    std::uint32_t selector_init = 0, selector_update = 0, selector_draw = 0;
    std::uint32_t bolt_init = 0, bolt_update = 0, bolt_draw = 0;
};

// Ratchet & Clank's double tap as the hero's update found it (Shim::explain_double_tap).
struct TapSnapshot {
    std::int16_t tap = 0, window = 0;
    std::int32_t item = 0, pending = 0, swap_item = 0, last_weapon = 0;
};

class Shim {
public:
    Game game;

    // The settings: Going Commando's option byte (pause while open), the click, and the HUD-open
    // request's countdown in Going Commando's frames (its own 4, or longer: "Hold before pausing").
    void set_options(bool pause, bool click, int open_delay_frames = kGcOpenDelay);
    // The level whose program is loaded (-1: none): its addresses, again only when it changed.
    void connect(int level);

    // The hook points.
    void update_play(const std::function<void()>& original);
    void freeze_update(const std::function<void()>& original);
    void freeze_draw(const std::function<void()>& original);
    void selector_init(std::uint32_t record);
    void selector_update(std::uint32_t record);
    int selector_draw(std::uint32_t record);

    // Whether the freeze mode is the mod's (its kind, entered by the copied gate, mode 7 on).
    bool in_wheel_freeze() const;
    // Whether the freeze kind is the mod's (whatever the mode).
    bool freeze_kind_is_ours() const;
    // Why the paused open cannot happen now (nullptr: it can).
    const char* paused_open_refusal() const;
    // The paused open off for the session: the freeze's tick hook did not run.
    bool paused_disabled() const { return m_paused_disabled; }
    // Whether the paused quick select can open in this level (its callbacks' addresses known).
    bool paused_path_available() const { return m_level.selector_update != 0; }
    const LevelAddresses& level() const { return m_level; }
    bool pause() const;
    bool click() const { return m_click; }
    bool pal() const { return !game.disc.ntsc; }

    // Ratchet & Clank's scale_ticks: n * frame_scale + 0.5, truncated.
    int ticks(int n) const;
    // Going Commando's frames in the game's ticks (the marked lines' gc_shim_frames).
    int frames(int n) const;

    // The layer's state, cleared (a level load, the mod's load).
    void reset();

    // --- For the polyfills (rac1_shim.cpp) ---
    void load_copies();
    void flush_copies();
    void fill_weapon_table();
    void rebuild_pages();
    void begin_paused(int arg);
    void end_paused(const char* reason);
    void quick_swap();
    void trace_request(int before, bool held, bool swap_flag);
    void explain_double_tap(const TapSnapshot& before);
    void queue_selector();
    void show_counters_health(int frames);
    void show_counters_bolts(int frames);
    void draw_ring(std::uint8_t* menu, int x, int y, int alpha);
    bool selector_open_allowed() const;
    // The gate's conditions, their values and the first that fails, for the log.
    std::string describe_gate() const;
    void log_once(const std::string& key, const std::string& line, bool warning = false);
    void* resolve_handle(std::int32_t handle) const;

    float math_sqrt(float v) const;
    float math_atan(float x, float y) const;
    float math_diff_rots(float a, float b) const;
    float math_sub_rots(float a, float b) const;
    float math_normalize(float a) const;
    float math_add_rots(float a, float b) const;
    float math_sin(float a) const;
    float math_cos(float a) const;
    float math_int_to_float(int v) const;
    int math_float_to_int(float v) const;
    std::uint32_t tween_colour(std::uint32_t from, std::uint32_t to, float t) const;

private:
    struct Snapshot {
        std::int32_t pending = 0;
        std::int16_t tap_timer = 0, tap_window = 0;
        std::uint16_t count = 0;
        std::uint32_t best = 0, levels = 0;
    };
    bool m_pause = true;
    bool m_click = true;
    int m_open_delay = kGcOpenDelay;
    int m_level_number = -2;
    LevelAddresses m_level;
    int m_copy_depth = 0;
    Snapshot m_seen;
    // The freeze the gate entered and is not yet over (its draw included).
    bool m_entered = false;
    // The HUD-open request, for the log: the play ticks, the state last seen, the tick it was asked
    // at, whether a swap set its flag.
    int m_play_ticks = 0;
    int m_request_tick = 0;
    std::string m_request_trace;
    // Records of Ratchet & Clank's own opener refused with the pause on (Going Commando's decider
    // opens the HUD quick select only with the option off).
    std::vector<std::uint32_t> m_refused;
    // The freeze's watchdog: its update ticks, its pictures since the last update, whether the
    // wheel's record opened in it.
    int m_freeze_ticks = 0;
    int m_draws_since_update = 0;
    bool m_record_opened = false;
    bool m_paused_disabled = false;
    std::vector<std::string> m_logged;

    friend class CopyScope;
};

Shim& shim();

// The copies loaded around a call of copied code (nested calls share the outermost's).
class CopyScope {
public:
    explicit CopyScope(Shim& s) : m_shim(s) {
        if (m_shim.m_copy_depth++ == 0) {
            m_shim.load_copies();
        }
    }
    ~CopyScope() {
        if (--m_shim.m_copy_depth == 0) {
            m_shim.flush_copies();
        }
    }
    CopyScope(const CopyScope&) = delete;
    CopyScope& operator=(const CopyScope&) = delete;

private:
    Shim& m_shim;
};

}  // namespace gc_weapon_wheel::gc

// The copied functions (src/gc/*.c) the layer and the mod call.
extern "C" {
void weapon_wheel_update_select(void* selector);
int weapon_wheel_draw(char* menu);
void menu_init_panel_layout(std::uint8_t* panel);
void weapon_menu_init_slots(void);
void UpdateModalState(void);
void menu_set_mode(std::int32_t mode, std::int32_t arg);
void menu_end(void);
int menu_is_state_seven(void);
void menu_advance_state_in_mode_seven(void);
void hud_request_letterbox_bars(void);
int level_update_game_frame_input(void);
void hero_update_button_hold_timers(void);
std::int32_t PadPressedWithin(std::uint32_t mask, std::int32_t window, std::int32_t* out);
void hero_update_hit_streak_effect(void);
void gfx_render_frame_extra_layers(void);
void func_0029CE38(void);

// The layer's buffers and state the copied code reads (gc/shim/common.h names them).
extern unsigned char gc_shim_hero[];
extern unsigned char gc_shim_stats[];
extern unsigned char gc_shim_weapon_table[];
extern unsigned char gc_shim_modal_state[];
extern unsigned char gc_shim_options[];
extern int gc_shim_modal_timer;
extern int gc_shim_modal_counter;
extern int gc_shim_hud_flag_a;
extern int gc_shim_hud_flag_b;
extern int gc_shim_press_window;
extern int gc_shim_input_block;
extern int gc_shim_open_delay_start;
int gc_shim_gate_window(int frames);
}
