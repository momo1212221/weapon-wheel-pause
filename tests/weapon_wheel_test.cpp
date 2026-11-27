// SPDX-License-Identifier: GPL-3.0-or-later
//
// The translation layer with the copied Going Commando code (src/gc/*.c) on a fake Ratchet & Clank:
// a game memory filled with a quick select's words and tables at the disc's addresses, and fake
// game functions that do what Ratchet & Clank's do with them (mode_freeze_init, queue_animation_update
// and update_hud_elements on one HUD record, the sound and music calls counted, the draws recorded).
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "gc/rac1_shim.hpp"
#include "layout.hpp"

namespace {

namespace gcw = gc_weapon_wheel;
namespace gc = gc_weapon_wheel::gc;

int g_failures = 0;
int g_checks = 0;

#define CHECK(cond)                                                                   \
    do {                                                                              \
        ++g_checks;                                                                   \
        if (!(cond)) {                                                                \
            ++g_failures;                                                             \
            std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                             \
    } while (0)

#define CHECK_EQ(a, b)                                                                              \
    do {                                                                                            \
        ++g_checks;                                                                                 \
        const long long va = static_cast<long long>(a), vb = static_cast<long long>(b);             \
        if (va != vb) {                                                                             \
            ++g_failures;                                                                           \
            std::fprintf(stderr, "%s:%d: CHECK_EQ failed: %s == %s (%lld vs %lld)\n", __FILE__, __LINE__, #a, #b, va, vb); \
        }                                                                                           \
    } while (0)

constexpr float kPi = 3.1415927f;
constexpr std::uint32_t kMemory = 0x200000;
// The quick select's HUD record (hud_chan_slots + 3 * 0x90 on PAL) and a place for the game's texts.
constexpr std::uint32_t kRecord = 0x00199C60u + 3 * 0x90;
constexpr std::uint32_t kTexts = 0x001E0000u;
constexpr std::uint32_t kStack = 0x001F0000u;
// The fake level's callbacks (as queue_animation_update stores them).
constexpr std::uint32_t kInitCode = 0x00238478u, kUpdateCode = 0x00238530u, kDrawCode = 0x00238C30u;

struct Sprite {
    int frame, x, y, w, h, alpha;
    std::uint32_t tint;
};
struct Text {
    int x, y;
    std::uint32_t colour;
    std::string text;
};

// The fake game.
struct Fake {
    std::vector<std::uint8_t> ram = std::vector<std::uint8_t>(kMemory, 0);
    gcw::Disc d{};
    gc::Memory m{};
    int handle_next = 100;
    int pause_group = 0, continue_group = 0, music_pauses = 0, music_unpauses = 0, flushes = 0, clicks = 0;
    int sound_updates = 0, play_ticks = 0, help_updates = 0, health_shown = 0, bolts_shown = 0;
    int removed = 0, freeze_inits = 0;
    std::uint32_t tint = UINT32_MAX;
    std::vector<Sprite> sprites;
    std::vector<Text> texts;
    std::vector<int> frames_drawn;
    int renders = 0;
    std::uint32_t next_text = kTexts;
    bool opener = false;       // Ratchet & Clank's own Triangle opener in the play tick
    bool rac1_timers = false;  // Ratchet & Clank's own double-tap timers in the play tick
    bool rac1_switch = false;  // Ratchet & Clank's hero_switch_item after them, the same tick
    bool match_code = true;    // whether the function table has the callbacks at their places
    bool block_selector = false;  // queue_animation_update refuses the quick select's record
    int level = 0;
    int swaps = 0;
    std::int32_t last_mode = 0;
    std::vector<std::string> logs;
    // The logs with the request trace's "(tap timer,window)" groups left out.
    static std::string plain(const std::string& l) {
        std::string out;
        int depth = 0;
        for (char c : l) {
            if (c == '(' && !out.empty() && (out.back() == 'H' || out.back() == '-')) {
                ++depth;
                continue;
            }
            if (depth > 0) {
                depth -= c == ')' ? 1 : 0;
                continue;
            }
            out += c;
        }
        return out;
    }
    bool traced(const char* what) const {
        for (const std::string& l : logs) {
            if (plain(l).find(what) != std::string::npos) {
                return true;
            }
        }
        return false;
    }
    int count_traced(const char* what) const {
        int n = 0;
        for (const std::string& l : logs) {
            n += plain(l).find(what) != std::string::npos ? 1 : 0;
        }
        return n;
    }
    bool logged(const char* what) const {
        for (const std::string& l : logs) {
            if (l.find(what) != std::string::npos) {
                return true;
            }
        }
        return false;
    }
    int count_logs(const char* what) const {
        int n = 0;
        for (const std::string& l : logs) {
            n += l.find(what) != std::string::npos ? 1 : 0;
        }
        return n;
    }

    template <class T> T rd(std::uint32_t a) const { return m.read<T>(a); }
    template <class T> void wr(std::uint32_t a, T v) const { m.write<T>(a, v); }

    std::uint32_t text(const char* s) {
        const std::uint32_t at = next_text;
        std::memcpy(ram.data() + at, s, std::strlen(s) + 1);
        next_text += static_cast<std::uint32_t>(std::strlen(s) + 1);
        return at;
    }

    // Ratchet & Clank's queue_animation_update on the one record of slot 3 (others: a handle).
    int queue(int key, int, std::uint32_t init, std::uint32_t update, std::uint32_t draw, std::uint32_t, int) {
        if (key != gc::kSelectorKey) {
            return handle_next++;
        }
        if (block_selector) {
            return 0;
        }
        if (rd<std::uint32_t>(kRecord + 0x30) == init && rd<std::uint32_t>(kRecord + 0x34) == update &&
            rd<std::uint32_t>(kRecord + 0x38) == draw) {
            return rd<std::int32_t>(kRecord + 0x64);
        }
        wr<std::uint32_t>(kRecord + 0x30, init);
        wr<std::uint32_t>(kRecord + 0x34, update);
        wr<std::uint32_t>(kRecord + 0x38, draw);
        wr<std::int32_t>(kRecord + 0x64, handle_next);
        wr<std::int32_t>(kRecord + 0x68, 1);
        wr<std::int32_t>(kRecord + 0x7C, 0);
        wr<std::int32_t>(kRecord + 0x70, 0);
        return handle_next++;
    }
    int remove(int handle) {
        if (rd<std::int32_t>(kRecord + 0x64) == handle) {
            wr<std::uint32_t>(kRecord + 0x30, 0xFFFF);
            wr<std::uint32_t>(kRecord + 0x34, 0);
            wr<std::uint32_t>(kRecord + 0x38, 0);
            ++removed;
            return 1;
        }
        return 0;
    }
    bool open() const { return rd<std::uint32_t>(kRecord + 0x34) != 0; }
    // update_hud_elements(1): the record's pending init, then its update.
    int update_hud(int) {
        if (rd<std::int32_t>(kRecord + 0x68) != 0) {
            wr<std::int32_t>(kRecord + 0x68, 0);
            gc::shim().selector_init(kRecord);
        }
        if (open()) {
            gc::shim().selector_update(kRecord);
        }
        return 0;
    }
    // mode_freeze_init as freeze.c:45 does it.
    void freeze_init(int kind, int, int) {
        ++freeze_inits;
        if (rd<std::int32_t>(d.game_mode) != 3) {
            ++pause_group;
            ++music_pauses;
        }
        wr<std::int32_t>(d.mode_freeze_info + 0x14, rd<std::int32_t>(d.game_mode));
        wr<std::int32_t>(d.game_mode, 4);
        wr<std::int32_t>(d.mode_freeze_info, kind);
    }
    // update_mode_freeze for a kind it has no case for: the sound, then the tail once the mode is left.
    void freeze_original() {
        ++sound_updates;
        const std::int32_t mode = rd<std::int32_t>(d.game_mode);
        if (mode != 3 && mode != 4) {
            ++continue_group;
            ++music_unpauses;
            ++flushes;
        }
    }
    // handle_pause_input: Ratchet & Clank's own Triangle opener (when asked), then the HUD's update.
    void play_original() {
        ++play_ticks;
        if (rac1_timers) {
            hero_timers();
        }
        if (rac1_switch) {
            hero_switch();
        }
        if (opener && (rd<std::uint32_t>(d.pad + gcw::kPadPressed) & 0x10) != 0) {
            const gcw::LevelCallbacks cb = gcw::level_callbacks(d.ntsc, level);
            queue(3, 0, cb.selector_init, cb.selector_update, cb.selector_draw, 0, 0);
        }
        update_hud(1);
    }
    // Ratchet & Clank's hero_switch_item for slot 0 (hero_animation_slots.c): a pending item other than
    // the one taken becomes it; the quick swap item takes the last weapon, the last weapon the new
    // item (here the hand changes at once).
    void hero_switch() {
        const std::uint32_t h = d.hero;
        const std::int32_t u = rd<std::int32_t>(h + gcw::kHeroPendingItem);
        if (u == 0) {
            return;
        }
        if (u != rd<std::int32_t>(h + gcw::kHeroRequestedItem)) {
            if (u != 0x24 && rd<std::int32_t>(h + gcw::kHeroItem) != 8) {
                wr<std::int32_t>(d.quick_swap_item, rd<std::int32_t>(d.last_weapon));
            }
            wr<std::int32_t>(h + gcw::kHeroRequestedItem, u);
            wr<std::int32_t>(d.last_weapon, u);
            wr<std::int32_t>(h + gcw::kHeroItem, u);
        }
        wr<std::int32_t>(h + gcw::kHeroPendingItem, 0);
    }
    // Ratchet & Clank's own double tap (hero_update_transitions, hero_item_input_and_hud.c:142-170).
    void hero_timers() {
        const std::uint32_t h = d.hero;
        const int twenty = d.ntsc ? 20 : 17;
        for (std::uint32_t t : {h + gcw::kHeroTapTimer, h + gcw::kHeroTapWindow}) {
            const std::int16_t v = rd<std::int16_t>(t);
            if (v != 0) {
                wr<std::int16_t>(t, static_cast<std::int16_t>(v - 1));
            }
        }
        if (rd<std::uint32_t>(d.pad + gcw::kPadPressed) & 0x10) {
            wr<std::int16_t>(h + gcw::kHeroTapTimer, static_cast<std::int16_t>(twenty));
        }
        if ((rd<std::int16_t>(h + gcw::kHeroTapTimer) != 0 || rd<std::int16_t>(h + gcw::kHeroTapWindow) != 0) &&
            (rd<std::uint32_t>(d.pad + gcw::kPadRawReleased) & 0x10)) {
            if (rd<std::int16_t>(h + gcw::kHeroTapWindow) == 0) {
                wr<std::int16_t>(h + gcw::kHeroTapWindow, static_cast<std::int16_t>(twenty));
            } else {
                const std::int32_t swap = rd<std::int32_t>(d.quick_swap_item);
                const std::int32_t item = rd<std::int32_t>(h + gcw::kHeroItem);
                const std::int32_t next = item == 8 ? rd<std::int32_t>(d.last_weapon) : swap;
                if (next != 0 && item != swap) {
                    wr<std::int32_t>(h + gcw::kHeroPendingItem, next);
                    wr<std::int16_t>(h + gcw::kHeroTapTimer, 0);
                    wr<std::int16_t>(h + gcw::kHeroTapWindow, 0);
                    ++swaps;
                }
            }
        }
    }

    // The pad, as UpdatePad leaves it after a poll: held and the edges, and the history.
    std::uint32_t last_held = 0;
    void poll(std::uint32_t held, float x = 0.0f, float y = 0.0f) {
        // The level's main loop: the ticks since the game mode changed start over at a change.
        const std::int32_t mode = rd<std::int32_t>(d.game_mode);
        if (mode != last_mode) {
            wr<std::int32_t>(d.cinematic_frames, 0);
            last_mode = mode;
        }
        const std::uint32_t pressed = held & ~last_held;
        const std::uint32_t released = last_held & ~held;
        last_held = held;
        const std::uint32_t p = d.pad;
        wr<std::uint32_t>(p + gcw::kPadHeld, held);
        wr<std::uint32_t>(p + gcw::kPadPressed, pressed);
        wr<std::uint32_t>(p + 0x1A8, released);
        wr<std::uint32_t>(p + gcw::kPadRawReleased, released);
        wr<std::uint32_t>(p + gcw::kPadHeldUnmasked, held);
        wr<std::uint32_t>(p + gcw::kPadPressedUnmasked, pressed);
        wr<float>(p + gcw::kPadStickX, x);
        wr<float>(p + gcw::kPadStickY, y);
        wr<std::int32_t>(p + gcw::kPadStickMoved, (x != 0.0f || y != 0.0f) ? 1 : 0);
        std::int16_t head = rd<std::int16_t>(p + gcw::kPadHistIdx);
        wr<std::uint32_t>(p + gcw::kPadHistory + 4u * static_cast<std::uint32_t>(head), pressed);
        wr<std::int16_t>(p + gcw::kPadHistIdx, static_cast<std::int16_t>((head + 1) % 30));
        const std::int32_t count = rd<std::int32_t>(p + gcw::kPadHistCount);
        wr<std::int32_t>(p + gcw::kPadHistCount, count < 30 ? count + 1 : 30);
        wr<std::int32_t>(d.cinematic_frames, rd<std::int32_t>(d.cinematic_frames) + 1);
        wr<std::int32_t>(d.level_frame_counter, rd<std::int32_t>(d.level_frame_counter) + 1);
        wr<std::int32_t>(d.rendered_frame_count, rd<std::int32_t>(d.rendered_frame_count) + 1);
    }
};

Fake* g_fake = nullptr;

// The slot assignment: eight items, slot 3 empty; item 0x18 in slot 7.
constexpr std::int32_t kItems[8] = {0x10, 0x11, 0x12, 0, 0x13, 0x14, 0x15, 0x18};

// exe_shift: the program loaded keeps the executable's game mode word that much further on (its
// own word left at the executable's address holds a 5).
void set_up(Fake& f, bool ntsc, bool pause, std::uint32_t exe_shift = 0) {
    g_fake = &f;
    gc::shim() = gc::Shim{};
    f.d = gcw::disc_for(ntsc);
    const std::uint32_t exe_mode = f.d.game_mode;
    f.d.game_mode += exe_shift;
    f.m = gc::Memory{f.ram.data()};
    const gcw::Disc& d = f.d;
    f.wr<float>(d.frame_scale, ntsc ? 1.0f : 50.0f / 60.0f);
    for (int i = 0; i < 8; ++i) {
        f.wr<std::int32_t>(d.slot_items + 4u * static_cast<std::uint32_t>(i), kItems[i]);
    }
    for (std::uint32_t id = 1; id < 0x20; ++id) {
        f.wr<std::uint16_t>(d.item_defs + id * gcw::kItemDefStride + gcw::kItemDefIcon, static_cast<std::uint16_t>(0xE900 + id));
        f.wr<std::int16_t>(d.item_defs + id * gcw::kItemDefStride + gcw::kItemDefName, static_cast<std::int16_t>(0x100 + id));
        f.wr<std::int16_t>(d.item_defs + id * gcw::kItemDefStride + gcw::kItemDefGoldName, static_cast<std::int16_t>(0x200 + id));
        f.wr<std::uint16_t>(d.item_price + id * gcw::kPriceStride + gcw::kPriceUsesAmmo, id == 0x12 ? 0 : 1);
        f.wr<std::uint16_t>(d.item_price + id * gcw::kPriceStride + gcw::kPriceMaxAmmo, 40);
        f.wr<std::int32_t>(d.ammo + 4 * id, 12);
    }
    // Page 0's entries: ring neighbours (left -1, right +1), up and down across the ring.
    for (std::uint32_t i = 0; i < 8; ++i) {
        const std::uint32_t e = d.page0_entries + i * 28;
        f.wr<std::int32_t>(e + 0x08, static_cast<std::int32_t>((i + 7) % 8));
        f.wr<std::int32_t>(e + 0x0C, static_cast<std::int32_t>((i + 1) % 8));
        f.wr<std::int32_t>(e + 0x10, 0);
        f.wr<std::int32_t>(e + 0x14, 4);
    }
    // The page table quick_select_page + 12 holds page 0's entries.
    f.wr<std::uint32_t>(d.page + 12, d.page0_entries);
    // Defaults with nothing selected: left 6, right 2, up 0, down 4.
    f.wr<std::int32_t>(d.default_left, 6);
    f.wr<std::int32_t>(d.default_left + 4, 2);
    f.wr<std::int32_t>(d.default_left + 8, 0);
    f.wr<std::int32_t>(d.default_left + 12, 4);
    std::memcpy(f.ram.data() + d.ammo_format, "%d/%d", 6);
    f.wr<std::int32_t>(d.width, 200);
    f.wr<std::int32_t>(d.height, 180);
    f.wr<std::int32_t>(d.center_x, 100);
    f.wr<std::int32_t>(d.center_y, 90);
    f.wr<float>(d.ring_radius, 70.0f);
    f.wr<float>(d.ring_aspect, 1.0f);
    f.wr<float>(d.unselected_alpha, 0.5f);
    f.wr<std::int32_t>(d.cinematic_frames, 100);
    f.wr<std::int32_t>(d.quick_swap_item, 0x11);
    f.wr<std::int32_t>(d.last_weapon, 0x13);
    f.wr<std::int32_t>(d.hero + gcw::kHeroItem, 0x10);
    f.wr<std::int32_t>(d.level_index, 3);
    f.wr<std::int32_t>(d.event_frames, 6000);
    // The record's place and size on the screen (hud_screen_position's input).
    f.wr<std::int32_t>(kRecord + 0x50, 256);
    f.wr<std::int32_t>(kRecord + 0x54, 224);
    f.wr<std::int32_t>(kRecord + 0x60, 0);

    gc::Shim& s = gc::shim();
    gc::Game g;
    g.memory = f.m;
    g.disc = d;
    g.relocate = [](std::uint32_t a) { return a; };
    static std::uint32_t s_exe_mode, s_shift;
    s_exe_mode = exe_mode;
    s_shift = exe_shift;
    if (exe_shift != 0) {
        f.wr<std::int32_t>(exe_mode, 5);
    }
    g.relocate_exe = [](std::uint32_t a) { return a == s_exe_mode ? a + s_shift : a; };
    g.code_matches = [](std::uint32_t address, const char* name) {
        const gcw::LevelCallbacks cb = gcw::level_callbacks(g_fake->d.ntsc, g_fake->level);
        const char* const* names = g_fake->d.ntsc ? gcw::kCallbackNamesNtsc : gcw::kCallbackNamesPal;
        const std::uint32_t at[6] = {cb.selector_init, cb.selector_update, cb.selector_draw,
                                     cb.bolt_init, cb.bolt_update, cb.bolt_draw};
        bool known = false;
        for (int i = 0; i < 6; ++i) {
            known = known || (at[i] == address && std::strcmp(names[i], name) == 0);
        }
        return g_fake->match_code && known;
    };
    g.mode_freeze_init = [](int k, int a, int b) { g_fake->freeze_init(k, a, b); };
    g.update_hud_elements = [](int w) { return g_fake->update_hud(w); };
    g.help_update = [] { ++g_fake->help_updates; };
    g.music_unpause = [] { ++g_fake->music_unpauses; };
    g.music_pause = [](int) { ++g_fake->music_pauses; };
    g.snd_pause_group = [](int group) {
        CHECK_EQ(group, 0x1D);
        ++g_fake->pause_group;
    };
    g.snd_continue_group = [](int group) {
        CHECK_EQ(group, 0x1D);
        ++g_fake->continue_group;
    };
    g.snd_flush = [] { ++g_fake->flushes; };
    g.draw_hud_sprite = [](int frame, int x, int y, int w, int h, int alpha) {
        g_fake->sprites.push_back({frame, x, y, w, h, alpha, g_fake->tint});
    };
    g.get_icon_frame = [](int icon, int variant) { return icon * 8 + variant; };
    g.hud_frame = [](std::uint32_t rec, int frame, int, int, std::uint32_t, int) {
        CHECK_EQ(rec, kRecord);
        g_fake->frames_drawn.push_back(frame);
    };
    g.font_print_center = [](int x, int y, std::uint32_t colour, const char* text, int length) {
        g_fake->texts.push_back({x, y, colour, length < 0 ? std::string(text) : std::string(text, static_cast<std::size_t>(length))});
    };
    g.msg_string = [](int id) -> std::uint32_t {
        char name[64];
        if (id == 0x110) {
            std::snprintf(name, sizeof name, "Bomb Glove");
        } else if (id == 0x111) {
            std::snprintf(name, sizeof name, "Pyro-citor");
        } else if (id == 0x112) {
            std::snprintf(name, sizeof name, "Suck");
        } else {
            std::snprintf(name, sizeof name, "Item%x", id);
        }
        return g_fake->text(name);
    };
    g.show_health_hud = [] {
        ++g_fake->health_shown;
        return 7;
    };
    g.hud_keep_visible = [](int, int) { return 0; };
    g.queue_animation_update = [](int key, int a, std::uint32_t i, std::uint32_t u, std::uint32_t dr, std::uint32_t data, int b) {
        if (key == 2) {
            ++g_fake->bolts_shown;
        }
        return g_fake->queue(key, a, i, u, dr, data, b);
    };
    g.remove_hud_item = [](int h) { return g_fake->remove(h); };
    g.append_gif_transfer_packet = [] {};
    g.draw_debug_profiler = [] {
        // The world's render: the HUD's draw callbacks.
        ++g_fake->renders;
        if (g_fake->open()) {
            gc::shim().selector_draw(kRecord);
        }
    };
    g.play_click = [] { ++g_fake->clicks; };
    g.hud_screen_position = [](std::uint32_t rec, int& x, int& y) {
        // Ratchet & Clank's (weapon_hud_and_counters.c): centred by the record's size with flags 0.
        x -= g_fake->rd<std::int32_t>(rec + 0x58) >> 1;
        y -= g_fake->rd<std::int32_t>(rec + 0x5C) >> 1;
    };
    g.log = [](const char* what) { g_fake->logs.push_back(std::string("WARN ") + what); };
    g.info = [](const char* what) { g_fake->logs.push_back(what); };
    s.game = g;
    s.set_options(pause, true);
    s.connect(-2);
    s.connect(f.level);
}

// A play tick: the pad polled, then the level's tick through the hook; the world's render after.
void play(Fake& f, std::uint32_t held, float x = 0.0f, float y = 0.0f) {
    f.poll(held, x, y);
    gc::shim().update_play([&] { f.play_original(); });
}

// Triangle pressed, then held, until the freeze is entered (at most 20 ticks): the ticks it took.
int hold_until_freeze(Fake& f) {
    int n = 0;
    while (f.rd<std::int32_t>(f.d.game_mode) == 0 && n < 20) {
        play(f, 0x10);
        ++n;
    }
    return n;
}

// Ratchet & Clank's letterbox words (letterbox_bar_height, letterbox_enabled): never written.
constexpr std::int32_t kLetterboxSentinel = 0x5A5A5A5A;
void mark_letterbox(Fake& f) {
    const std::uint32_t at = f.d.ntsc ? 0x0015F444u : 0x0015F544u;
    f.wr<std::int32_t>(at, kLetterboxSentinel);
    f.wr<std::int32_t>(at + 4, kLetterboxSentinel);
}
bool letterbox_untouched(const Fake& f) {
    const std::uint32_t at = f.d.ntsc ? 0x0015F444u : 0x0015F544u;
    return f.rd<std::int32_t>(at) == kLetterboxSentinel && f.rd<std::int32_t>(at + 4) == kLetterboxSentinel;
}

// A freeze tick: the pad polled, the freeze mode's update and picture through the hooks.
void freeze(Fake& f, std::uint32_t held, float x = 0.0f, float y = 0.0f) {
    f.poll(held, x, y);
    gc::shim().freeze_update([&] { f.freeze_original(); });
    gc::shim().freeze_draw([] {});
}

std::int32_t selected(const Fake& f) { return f.rd<std::int32_t>(kRecord + gcw::kRecordSelected); }
std::uint8_t fade(const Fake& f, int i) { return f.rd<std::uint8_t>(kRecord + gcw::kRecordFades + static_cast<std::uint32_t>(i)); }
std::int32_t modal(std::uint32_t offset) {
    std::int32_t v;
    std::memcpy(&v, gc_shim_modal_state + offset, sizeof v);
    return v;
}

// The stick at `angle` (the angle fast_arc_tan(x, y) gives: atan2(y, x) here), full tilt.
void stick_at(float angle, float& x, float& y) {
    x = std::cos(angle);
    y = std::sin(angle);
}

// --- Tests ---

void test_frames_and_ticks() {
    Fake f;
    set_up(f, false, true);
    gc::Shim& s = gc::shim();
    CHECK_EQ(s.frames(180), 150);
    CHECK_EQ(s.frames(30), 25);
    CHECK_EQ(s.frames(20), 17);
    CHECK_EQ(s.frames(10), 8);
    CHECK_EQ(s.frames(6), 5);
    CHECK_EQ(s.ticks(180), 150);
    CHECK_EQ(s.ticks(360), 300);
    Fake n;
    set_up(n, true, true);
    CHECK_EQ(s.frames(180), 180);
    CHECK_EQ(s.frames(20), 20);
    CHECK_EQ(s.ticks(360), 360);
    // frame_scale out of range (NTSC's address unchecked): the tick rate's factor.
    n.wr<float>(n.d.frame_scale, 0.0f);
    CHECK_EQ(s.ticks(360), 360);
}

void test_pad_history_and_press_window() {
    Fake f;
    set_up(f, true, true);
    f.poll(0x10);
    f.poll(0x10);
    f.poll(0);
    f.poll(0x40);
    std::int32_t age = -1;
    // Triangle was pressed 3 polls back (the newest first): within 4 polls, not within 3.
    CHECK_EQ(PadPressedWithin(0x10, 3, &age), 0);
    CHECK_EQ(PadPressedWithin(0x10, 4, &age), 0x10);
    CHECK_EQ(age, 3);
    CHECK_EQ(PadPressedWithin(0x40, 1, &age), 0x40);
    CHECK_EQ(age, 0);
    // The window: +1 a tick up to 10, 0 while input is blocked.
    gc_shim_press_window = 0;
    for (int i = 0; i < 15; ++i) {
        hero_update_hit_streak_effect();
    }
    CHECK_EQ(gc_shim_press_window, 10);
    gc_shim_input_block = 1;
    hero_update_hit_streak_effect();
    CHECK_EQ(gc_shim_press_window, 0);
    gc_shim_input_block = 0;
}

// The HUD quick select ("Pause while open" off): Ratchet & Clank's own Triangle opens it, Going
// Commando's code runs it, no freeze.
void open_hud(Fake& f) {
    f.opener = true;
    play(f, 0x10);
    CHECK(f.open());
    // Going Commando's quick select takes the stick once its HUD-open request is done: Triangle held
    // its few frames.
    for (int i = 0; i < 6; ++i) {
        play(f, 0x10);
    }
    CHECK_EQ(gc_shim_hud_flag_b, 3);
    CHECK_EQ(f.rd<std::int32_t>(f.d.game_mode), 0);
}

void test_selection_math() {
    Fake f;
    set_up(f, true, false);
    open_hud(f);
    // menu_init_panel_layout: 210 x 200, nothing selected (-2), arming countdown, the slots filled.
    CHECK_EQ(f.rd<std::int32_t>(kRecord + 0x58), 210);
    CHECK_EQ(f.rd<std::int32_t>(kRecord + 0x5C), 200);
    CHECK_EQ(f.rd<std::int32_t>(f.d.slot_count), 8);
    CHECK_EQ(f.rd<std::uint32_t>(f.d.slot_tables), f.d.page + 12);
    CHECK_EQ(f.rd<std::int32_t>(f.d.page0_entries + 0x18), 0x10);
    CHECK_EQ(f.rd<std::int32_t>(f.d.page0_entries + 0), 0xE910);
    CHECK_EQ(f.rd<std::int32_t>(f.d.page0_entries + 3 * 28), 0);
    // The stick idle arms it: nothing selected.
    play(f, 0x10);
    CHECK_EQ(selected(f), -1);
    CHECK_EQ(f.rd<std::int32_t>(kRecord + 0x78), 0x10000FF);
    float x, y;
    // Up (y -1): slot 0; right: slot 2; down: slot 4; left: slot 6; up-right: slot 1.
    struct { float angle; int slot; } cases[] = {
        {-kPi / 2, 0}, {0.0f, 2}, {kPi / 2, 4}, {kPi, 6}, {-kPi / 4, 1}};
    for (const auto& c : cases) {
        play(f, 0x10);  // a frame with the stick back: the selection stays (hysteresis needs a move)
        stick_at(c.angle, x, y);
        // From -1 (re-arm by a release of the stick: magnitude under 0.5 resets to -1 when unarmed).
        f.wr<std::int32_t>(kRecord + gcw::kRecordSelected, -1);
        play(f, 0x10, x, y);
        CHECK_EQ(selected(f), c.slot);
    }
    // Hysteresis: from slot 2 (right), 0.5 rad on stays, 0.7 rad on goes to 1 or 3.
    stick_at(0.0f, x, y);
    f.wr<std::int32_t>(kRecord + gcw::kRecordSelected, -1);
    play(f, 0x10, x, y);
    CHECK_EQ(selected(f), 2);
    stick_at(0.5f, x, y);
    play(f, 0x10, x, y);
    CHECK_EQ(selected(f), 2);
    stick_at(-0.7f, x, y);
    play(f, 0x10, x, y);
    CHECK_EQ(selected(f), 1);
    // The even slots' snap: a stick just past slot 2's edge (fraction 0.1 of its sector) goes to 1.
    const float shift = 2 * kPi + kPi / 2 + kPi / 8;
    const float edge = 10.1f / 8.0f * 2 * kPi - shift;
    stick_at(edge, x, y);
    f.wr<std::int32_t>(kRecord + gcw::kRecordSelected, -1);
    play(f, 0x10, x, y);
    CHECK_EQ(selected(f), 1);
    // The nudge: slot 3 is empty; a stick in its sector past its middle goes on to 4, before it to 2.
    stick_at(11.7f / 8.0f * 2 * kPi - shift, x, y);
    f.wr<std::int32_t>(kRecord + gcw::kRecordSelected, -1);
    play(f, 0x10, x, y);
    CHECK_EQ(selected(f), 4);
    stick_at(11.3f / 8.0f * 2 * kPi - shift, x, y);
    f.wr<std::int32_t>(kRecord + gcw::kRecordSelected, -1);
    play(f, 0x10, x, y);
    CHECK_EQ(selected(f), 2);
    // A click on each change (the setting on).
    CHECK(f.clicks > 0);
}

void test_dpad_neighbours() {
    Fake f;
    set_up(f, true, false);
    open_hud(f);
    play(f, 0x10);
    CHECK_EQ(selected(f), -1);
    // Nothing selected: the default up (0), then the page's neighbours.
    play(f, 0x10 | 0x1000);
    CHECK_EQ(selected(f), 0);
    play(f, 0x10);
    play(f, 0x10 | 0x2000);  // right of 0: 1
    CHECK_EQ(selected(f), 1);
    play(f, 0x10);
    play(f, 0x10 | 0x4000);  // down of 1: 4
    CHECK_EQ(selected(f), 4);
    play(f, 0x10);
    play(f, 0x10 | 0x8000);  // left of 4: 3, empty: stays (no nudge with the d-pad)
    CHECK_EQ(selected(f), 3);
    // Nothing selected: the default left.
    f.wr<std::int32_t>(kRecord + gcw::kRecordSelected, -1);
    play(f, 0x10);
    play(f, 0x10 | 0x8000);
    CHECK_EQ(selected(f), 6);
}

void test_fades_and_commit() {
    Fake f;
    set_up(f, true, false);
    open_hud(f);
    play(f, 0x10);
    for (int i = 0; i < 10; ++i) {
        play(f, 0x10);
    }
    CHECK_EQ(fade(f, 0), 8);
    // A new selection: the selection fade starts over, then grows to 8.
    play(f, 0x10 | 0x2000);  // the default right: 2
    CHECK_EQ(selected(f), 2);
    CHECK_EQ(fade(f, 1), 0);
    for (int i = 0; i < 10; ++i) {
        play(f, 0x10);
    }
    CHECK_EQ(fade(f, 1), 8);
    f.wr<std::int32_t>(f.d.ammo + 4 * 0x12, 55);  // more than the 40 it can hold (and 0x12 uses none)
    f.wr<std::int32_t>(f.d.ammo + 4 * 0x10, 55);
    const std::uint32_t usage = f.d.usage_stats + gcw::kUsageStride * gcw::kUsageQuickSelect;
    // Let go: the open fade counts down, the commit at 0.
    for (int i = 0; i < 7; ++i) {
        play(f, 0);
        CHECK_EQ(f.rd<std::int32_t>(f.d.hero + gcw::kHeroPendingItem), 0);
    }
    play(f, 0);
    CHECK_EQ(f.rd<std::int32_t>(f.d.hero + gcw::kHeroPendingItem), 0x12);
    CHECK_EQ(f.rd<std::uint16_t>(usage), 1);
    CHECK_EQ(f.rd<std::uint16_t>(usage + 2), 6000 / 600);
    CHECK_EQ(f.rd<std::uint32_t>(usage + 4), (1u << 3) | 0x80000000u);
    CHECK_EQ(f.rd<std::int32_t>(f.d.ammo + 4 * 0x12), 55);  // no ammo: not clamped
    CHECK(!f.open());
    CHECK_EQ(f.rd<std::int32_t>(kRecord + gcw::kRecordSlide), -6);
    CHECK_EQ(f.rd<std::int32_t>(f.d.saved_selected), 2);
    CHECK_EQ(f.rd<std::int32_t>(f.d.close_frame), f.rd<std::int32_t>(f.d.rendered_frame_count));

    // Item 0x18 (slot 7): hero+0x1FF5, no pending item (Ratchet & Clank's rule).
    f.wr<std::int32_t>(f.d.hero + gcw::kHeroPendingItem, 0);
    open_hud(f);
    play(f, 0x10);
    play(f, 0x10 | 0x8000);  // the default left: 6
    play(f, 0x10);
    play(f, 0x10 | 0x2000);  // right of 6: 7
    CHECK_EQ(selected(f), 7);
    for (int i = 0; i < 10; ++i) {
        play(f, 0);
    }
    CHECK_EQ(f.rd<std::uint8_t>(f.d.hero + gcw::kHeroItem18), 1);
    CHECK_EQ(f.rd<std::int32_t>(f.d.hero + gcw::kHeroPendingItem), 0);

    // An ammo weapon is clamped to its most on the commit.
    open_hud(f);
    play(f, 0x10);
    play(f, 0x10 | 0x1000);  // the default up: 0, item 0x10
    for (int i = 0; i < 10; ++i) {
        play(f, 0);
    }
    CHECK_EQ(f.rd<std::int32_t>(f.d.hero + gcw::kHeroPendingItem), 0x10);
    CHECK_EQ(f.rd<std::int32_t>(f.d.ammo + 4 * 0x10), 40);
}

void test_eqs_paused_page_commit() {
    Fake f;
    set_up(f, true, true);
    unsigned begins = 0, ends = 0;
    bool second = false;
    gc::shim().game.eqs_page = [&](uint32_t event, uint32_t held, eqs_wheel_page_v2* out) -> int {
        if (event == EQS_WHEEL_END) { ++ends; return 1; }
        if (!out) return 0;
        if (event == EQS_WHEEL_BEGIN) { ++begins; second = false; }
        if (event == EQS_WHEEL_UPDATE && (held & 0x08)) second = true;
        out->page_count = 2;
        out->page_index = second ? 1 : 0;
        // Slot 2 has a different weapon than the native page.
        out->native_ids[2] = 0x11;
        return 1;
    };
    const auto canonical = f.rd<std::int32_t>(f.d.slot_items + 8);
    CHECK_EQ(canonical, 0x12);
    hold_until_freeze(f);
    for (int i = 0; i < 12; ++i) freeze(f, 0x10);
    CHECK_EQ(begins, 1u);
    freeze(f, 0x10 | 0x08);
    freeze(f, 0x10 | 0x2000);
    for (int i = 0; i < 12; ++i) freeze(f, 0x10);
    CHECK_EQ(selected(f), 2);
    CHECK_EQ(f.rd<std::int32_t>(f.d.slot_items + 8), canonical);
    CHECK_EQ(f.rd<std::int32_t>(f.d.page0_entries + 2*28 + 0x18), canonical);
    f.texts.clear();
    gc::shim().selector_draw(kRecord);
    CHECK(!f.texts.empty() && f.texts.back().text == "L1  PAGE 2 / 2  R1");
    for (int i = 0; i < 20 && gc::shim().in_wheel_freeze(); ++i) freeze(f, 0);
    for (int i = 0; i < 12; ++i) play(f, 0);
    CHECK_EQ(f.rd<std::int32_t>(f.d.hero + gcw::kHeroPendingItem), 0x11);
    CHECK_EQ(f.rd<std::int32_t>(f.d.slot_items + 8), canonical);
    CHECK_EQ(ends, 1u);
    gc::shim().game.eqs_page = {};
}

void test_draw() {
    Fake f;
    set_up(f, true, false);
    open_hud(f);
    play(f, 0x10);
    play(f, 0x10 | 0x2000);  // slot 2: item 0x12, no ammo, "Suck"
    for (int i = 0; i < 10; ++i) {
        play(f, 0x10);
    }
    f.sprites.clear();
    f.texts.clear();
    f.frames_drawn.clear();
    const int w = gc::shim().selector_draw(kRecord);
    CHECK_EQ(w, 210);
    // The ring: four panel quarters at the menu's alpha, centred on Going Commando's panel.
    CHECK(f.sprites.size() >= 5);
    CHECK_EQ(f.sprites[0].frame, gcw::kPanelSprite * 8);
    CHECK_EQ(f.sprites[0].alpha, 128);
    // At the record's place (256, 224) as Ratchet & Clank's hud_screen_position anchors Going
    // Commando's 210 x 200 panel; Ratchet & Clank's ring (200 x 180) centred on it.
    CHECK_EQ(f.sprites[0].x, 256 - 105 + (210 - 200) / 2);
    CHECK_EQ(f.sprites[0].y, 224 - 100 + (200 - 180) / 2);
    // The marker at the selection, pulsing as Ratchet & Clank's (its alpha times 0.75..1).
    bool marker = false;
    for (const Sprite& s : f.sprites) {
        if (s.frame == gcw::kMarkerSprite * 8) {
            marker = true;
            CHECK(s.alpha > 0 && s.alpha <= 128);
        }
    }
    CHECK(marker);
    // Seven icons (slot 3 is empty).
    CHECK_EQ(f.frames_drawn.size(), 7u);
    // No ammo: the name alone, one line, at y + 100, centred at x + 105.
    CHECK_EQ(f.texts.size(), 1u);
    CHECK(f.texts[0].text == "Suck");
    CHECK_EQ(f.texts[0].x, 256);
    CHECK_EQ(f.texts[0].y, 224 - 100 + 100);
    CHECK_EQ(f.texts[0].colour, 0x80f0f0f0u);

    // An ammo weapon: "12/40" at y + 112, the name at y + 75, split at '-' onto +16.
    play(f, 0x10 | 0x8000);  // left of 2: 1, item 0x11 "Pyro-citor"
    for (int i = 0; i < 10; ++i) {
        play(f, 0x10);
    }
    f.texts.clear();
    gc::shim().selector_draw(kRecord);
    CHECK_EQ(f.texts.size(), 3u);
    if (f.texts.size() == 3) {
        CHECK(f.texts[0].text == "12/40");
        CHECK_EQ(f.texts[0].y, 124 + 112);
        CHECK_EQ(f.texts[0].colour, 0x80f0f0f0u);
        CHECK(f.texts[1].text == "Pyro-");
        CHECK_EQ(f.texts[1].y, 124 + 75);
        CHECK(f.texts[2].text == "citor");
        CHECK_EQ(f.texts[2].y, 124 + 75 + 16);
    }
    // No ammo left: red.
    f.wr<std::int32_t>(f.d.ammo + 4 * 0x11, 0);
    f.texts.clear();
    gc::shim().selector_draw(kRecord);
    CHECK(!f.texts.empty() && f.texts[0].text == "0/40" && f.texts[0].colour == 0x804040ffu);
    // A gold weapon: Ratchet & Clank's gold name and icon frame.
    f.wr<std::uint8_t>(f.d.gold + 0x11, 1);
    f.texts.clear();
    f.frames_drawn.clear();
    gc::shim().selector_draw(kRecord);
    CHECK(!f.texts.empty() && f.texts.back().text == "Item211");
    CHECK_EQ(f.frames_drawn[1], (0xE900 + 0x11) * 8 + 4);
}

void test_paused_path() {
    Fake f;
    set_up(f, false, true);  // PAL: the frame counts scaled
    for (int i = 0; i < 12; ++i) {
        play(f, 0);
    }
    CHECK_EQ(gc_shim_press_window, 10);
    mark_letterbox(f);
    // Triangle pressed and held: the HUD-open request counts its 4 frames (3 ticks) down, then the
    // gate opens game mode 4, mode 7; that play tick is skipped.
    const int ticks_before = f.play_ticks;
    const int held = hold_until_freeze(f);
    CHECK_EQ(held, 5);
    CHECK_EQ(f.play_ticks, ticks_before + held - 1);
    CHECK_EQ(f.rd<std::int32_t>(f.d.game_mode), 4);
    CHECK(f.traced("done: the paused open after 4 ticks (state before/after, Triangle held, tap timer, window): 01H 12H 22H 22H 23H"));
    CHECK_EQ(f.rd<std::int32_t>(f.d.mode_freeze_info), gc::kFreezeKind);
    CHECK_EQ(f.rd<std::int32_t>(f.d.mode_freeze_info + 0x14), 0);
    CHECK_EQ(modal(gc::kModalMode), 7);
    CHECK_EQ(modal(gc::kModalWorldDrawn), 1);
    CHECK(gc::shim().in_wheel_freeze());
    // mode_freeze_init paused the music; mode 7 does not: unpaused after it.
    CHECK_EQ(f.music_pauses, 1);
    CHECK_EQ(f.music_unpauses, 1);
    CHECK_EQ(f.pause_group, 2);  // mode_freeze_init's, and menu_set_mode's cmd22(0x5D)
    // Phase 0, then 1: the wheel queued and updated in the freeze (update_hud_elements(1)), the
    // health meter and bolt counter shown.
    freeze(f, 0x10);
    CHECK_EQ(modal(gc::kModalPhase), 1);
    CHECK(!f.open());
    freeze(f, 0x10);
    CHECK(f.open());
    CHECK_EQ(modal(gc::kModalPhase), 1);
    CHECK(f.health_shown > 0 && f.bolts_shown > 0);
    CHECK(f.help_updates > 0);
    CHECK_EQ(f.sound_updates, 2);
    // Held: phase 1 holds (the layer keeps g_modal_timer armed), the world too.
    float x, y;
    stick_at(0.0f, x, y);
    for (int i = 0; i < 30; ++i) {
        freeze(f, 0x10, i < 2 ? 0.0f : x, i < 2 ? 0.0f : y);
    }
    CHECK_EQ(modal(gc::kModalPhase), 1);
    CHECK_EQ(f.rd<std::int32_t>(f.d.game_mode), 4);
    CHECK_EQ(selected(f), 2);
    CHECK_EQ(f.play_ticks, ticks_before + held - 1);
    // Let go: phase 2 counts down, phase 3 ends: back to play, the sounds and the music on, the
    // pad's history cleared.
    const int unpauses = f.music_unpauses;
    int n = 0;
    while (f.rd<std::int32_t>(f.d.game_mode) == 4 && n < 20) {
        freeze(f, 0);
        ++n;
    }
    CHECK(n >= 2 && n < 20);
    CHECK_EQ(f.rd<std::int32_t>(f.d.game_mode), 0);
    CHECK_EQ(modal(gc::kModalMode), 0);
    CHECK(f.continue_group >= 1);
    CHECK(f.music_unpauses > unpauses);
    CHECK(f.flushes >= 1);
    CHECK_EQ(f.rd<std::int32_t>(f.d.pad + gcw::kPadHistCount), 0);
    // The wheel fades out in play and commits there.
    for (int i = 0; i < 12; ++i) {
        play(f, 0);
    }
    CHECK_EQ(f.rd<std::int32_t>(f.d.hero + gcw::kHeroPendingItem), 0x12);
    CHECK(!f.open());
    // Going Commando's bars are left out: Ratchet & Clank's letterbox is never written.
    CHECK(letterbox_untouched(f));
    // No second open from the same press: the history was cleared.
    CHECK_EQ(f.rd<std::int32_t>(f.d.game_mode), 0);
}

void test_gate_conditions() {
    Fake f;
    set_up(f, true, true);
    for (int i = 0; i < 12; ++i) {
        play(f, 0);
    }
    // A refused press stays in the pad's history for the window: cleared before the next case.
    auto forget = [&] { f.wr<std::int32_t>(f.d.pad + gcw::kPadHistCount, 0); };
    // In Clank's mode (1): Ratchet & Clank's four-slot radial is the game's; no paused open.
    f.wr<std::uint8_t>(f.d.hero + gcw::kHeroVehicleMode, 1);
    play(f, 0x10);
    CHECK_EQ(f.rd<std::int32_t>(f.d.game_mode), 0);
    forget();
    f.wr<std::uint8_t>(f.d.hero + gcw::kHeroVehicleMode, 0);
    play(f, 0);
    // The slot locked, then fewer than 8 ticks since the level's mode changed: no open.
    f.wr<std::uint8_t>(f.d.hero + gcw::kHeroSwitchLock, 2);
    play(f, 0x10);
    CHECK_EQ(f.rd<std::int32_t>(f.d.game_mode), 0);
    forget();
    f.wr<std::uint8_t>(f.d.hero + gcw::kHeroSwitchLock, 0);
    play(f, 0);
    f.wr<std::int32_t>(f.d.cinematic_frames, 5);
    play(f, 0x10);
    CHECK_EQ(f.rd<std::int32_t>(f.d.game_mode), 0);
    CHECK_EQ(gc_shim_hud_flag_b, 0);
    // The press is still within the window when the frames reach 8: the request starts then, and
    // the freeze once it is done, Triangle held.
    for (int i = 0; i < 10 && f.rd<std::int32_t>(f.d.game_mode) == 0; ++i) {
        play(f, 0x10);
    }
    CHECK_EQ(f.rd<std::int32_t>(f.d.game_mode), 4);
}

// The executable's game mode word where the level's program keeps it (Mod::relocate): read and
// written there, not at the executable's address.
void test_relocated_game_mode() {
    Fake f;
    set_up(f, true, true, 0x40);
    for (int i = 0; i < 12; ++i) {
        play(f, 0);
    }
    hold_until_freeze(f);
    CHECK_EQ(f.rd<std::int32_t>(f.d.game_mode), 4);
    CHECK_EQ(f.rd<std::int32_t>(gcw::disc_for(true).game_mode), 5);
    CHECK(gc::shim().in_wheel_freeze());
}

// Each level's own callbacks, from the catalogues' places, checked against the function table.
void test_level_callbacks() {
    Fake f;
    f.level = 17;
    set_up(f, true, true);
    CHECK_EQ(gc::shim().level().selector_init, 0x00232DD0u);
    CHECK_EQ(gc::shim().level().selector_update, 0x00232E88u);
    CHECK_EQ(gc::shim().level().selector_draw, 0x00233588u);
    CHECK_EQ(gc::shim().level().bolt_draw, 0x002350E0u);
    CHECK(gc::shim().paused_path_available());
    CHECK(f.logged("level 17: the HUD callbacks found"));
    Fake p;
    p.level = 18;
    set_up(p, false, true);
    CHECK_EQ(gc::shim().level().selector_init, 0x0023B9E8u);
    Fake n;
    n.level = 17;
    n.match_code = false;
    set_up(n, true, true);
    CHECK(!gc::shim().paused_path_available());
    CHECK(n.logged("WARN level 17: the HUD callbacks NOT all found"));
    CHECK(n.logged("NOT that function"));
}

// "Pause while open" on: the game's own Triangle opener is always refused; a tap does nothing and
// never freezes; a release before the request is done cancels it; a double tap within 20 ticks is
// the game's own quick swap, once.
void test_taps() {
    Fake f;
    f.opener = true;
    f.rac1_timers = true;
    set_up(f, true, true);
    for (int i = 0; i < 25; ++i) {
        play(f, 0);
    }
    // A tap: refused quick select, request cancelled as Triangle is let go, no freeze, no switch.
    play(f, 0x10);
    CHECK(!f.open());
    CHECK(f.logged("the game's own quick select refused"));
    CHECK_EQ(gc_shim_hud_flag_b, 1);
    play(f, 0);
    for (int i = 0; i < 25; ++i) {
        play(f, 0);
    }
    CHECK_EQ(f.freeze_inits, 0);
    CHECK_EQ(gc_shim_hud_flag_b, 0);
    CHECK(f.logged("cancelled: Triangle let go"));
    CHECK_EQ(f.rd<std::int32_t>(f.d.hero + gcw::kHeroPendingItem), 0);
    CHECK_EQ(f.swaps, 0);
    // Held a little, let go before the request is done: cancelled, no freeze.
    play(f, 0x10);
    play(f, 0x10);
    play(f, 0x10);
    play(f, 0);
    for (int i = 0; i < 25; ++i) {
        play(f, 0);
    }
    CHECK_EQ(f.freeze_inits, 0);
    CHECK_EQ(f.swaps, 0);
    // A double tap: the swap, once; the request cancelled; no freeze.
    play(f, 0x10);
    play(f, 0);
    play(f, 0x10);
    play(f, 0);
    for (int i = 0; i < 25; ++i) {
        play(f, 0);
    }
    CHECK_EQ(f.swaps, 1);
    CHECK_EQ(f.rd<std::int32_t>(f.d.hero + gcw::kHeroPendingItem), 0x11);
    CHECK_EQ(f.freeze_inits, 0);
    // (The gate asks again each tick the press is within its window, so the request ends for good
    // once the presses are out of it.)
    CHECK(f.count_logs("cancelled: Triangle let go") >= 3);
    CHECK(f.logged("double tap: swap from item 0x10 to 0x11 (the quick swap item"));
    // Held past the request's frames: the freeze, once.
    f.wr<std::int32_t>(f.d.hero + gcw::kHeroPendingItem, 0);
    const int held = hold_until_freeze(f);
    CHECK_EQ(held, 6);  // NTSC: 4 frames counted after the tick it is asked
    CHECK_EQ(f.freeze_inits, 1);
    // Let go at once, nothing selected: the freeze ends, nothing committed.
    int n = 0;
    while (f.rd<std::int32_t>(f.d.game_mode) == 4 && n < 20) {
        freeze(f, 0);
        ++n;
    }
    CHECK(n < 10);
    CHECK(!f.logged("watchdog"));
    for (int i = 0; i < 12; ++i) {
        play(f, 0);
    }
    CHECK_EQ(f.rd<std::int32_t>(f.d.hero + gcw::kHeroPendingItem), 0);
    CHECK_EQ(f.swaps, 1);
    CHECK_EQ(gc_shim_hud_flag_b, 0);
}

// The paused open not possible (the callbacks not found), the pause on: nothing opens; the double
// tap is still the game's.
void test_unavailable_paused_open() {
    Fake f;
    f.level = 17;
    f.match_code = false;
    f.opener = true;
    f.rac1_timers = true;
    set_up(f, true, true);
    for (int i = 0; i < 12; ++i) {
        play(f, 0);
    }
    for (int i = 0; i < 10; ++i) {
        play(f, 0x10);
    }
    CHECK_EQ(f.rd<std::int32_t>(f.d.game_mode), 0);
    CHECK(!f.open());
    CHECK(f.logged("Triangle: the gate does not open: the quick select's callbacks were not found"));
    play(f, 0);
    for (int i = 0; i < 25; ++i) {
        play(f, 0);
    }
    play(f, 0x10);
    play(f, 0);
    play(f, 0x10);
    play(f, 0);
    CHECK_EQ(f.swaps, 1);
    CHECK_EQ(f.count_logs("Triangle: "), 3);
}

// The freeze's tick hook bypassed (a port that calls the game's update_mode_freeze directly): two
// pictures with no tick end the freeze, the paused open goes off. The picture of the mod's kind is
// Going Commando's, not the dialog.
void test_watchdog_update_bypassed() {
    Fake f;
    set_up(f, true, true);
    for (int i = 0; i < 12; ++i) {
        play(f, 0);
    }
    hold_until_freeze(f);
    CHECK_EQ(f.rd<std::int32_t>(f.d.game_mode), 4);
    bool dialog = false;
    for (int i = 0; i < 2; ++i) {
        f.poll(0x10);
        gc::shim().freeze_draw([&] { dialog = true; });
    }
    CHECK(!dialog);
    CHECK_EQ(f.rd<std::int32_t>(f.d.game_mode), 0);
    CHECK(gc::shim().paused_disabled());
    CHECK(f.logged("WARN rac1.freeze.update did not run"));
    CHECK(f.logged("WARN the paused quick select ended by its watchdog: rac1.freeze.update did not run"));
    for (int i = 0; i < 12; ++i) {
        play(f, 0);
    }
    for (int i = 0; i < 10; ++i) {
        play(f, 0x10);
    }
    CHECK_EQ(f.rd<std::int32_t>(f.d.game_mode), 0);
    // Another freeze kind is the game's: its own picture.
    f.wr<std::int32_t>(f.d.mode_freeze_info, 2);
    dialog = false;
    gc::shim().freeze_draw([&] { dialog = true; });
    CHECK(dialog);
}

// The wheel's record never opens in the freeze: the watchdog ends it.
void test_watchdog_record_never_opens() {
    Fake f;
    f.block_selector = true;
    set_up(f, false, true);
    for (int i = 0; i < 12; ++i) {
        play(f, 0);
    }
    hold_until_freeze(f);
    CHECK_EQ(f.rd<std::int32_t>(f.d.game_mode), 4);
    int n = 0;
    while (f.rd<std::int32_t>(f.d.game_mode) == 4 && n < 60) {
        freeze(f, 0x10);
        ++n;
    }
    // Going Commando's own phases end it: with no quick select to set its timer again
    // (menu_set_state_word_three), phase 1 ends after 3 frames and the counter after 6.
    CHECK_EQ(f.rd<std::int32_t>(f.d.game_mode), 0);
    CHECK(n < 12);
    CHECK(!f.logged("watchdog"));
    CHECK(f.continue_group >= 1);
}

// A held press opens at the same tick every time (NTSC: the state machine's 4 frames after the
// tick it is asked, tick 5), and a request ends only on a real release.
void test_hold_timing() {
    Fake f;
    f.opener = true;
    f.rac1_timers = true;
    set_up(f, true, true);
    for (int k = 0; k < 3; ++k) {
        for (int i = 0; i < 30; ++i) {
            play(f, 0);
        }
        const int logs = f.count_traced("after 5 ticks (state before/after, Triangle held, tap timer, window): 01H 12H 22H 22H 22H 23H");
        CHECK_EQ(hold_until_freeze(f), 6);
        CHECK_EQ(f.count_traced("after 5 ticks (state before/after, Triangle held, tap timer, window): 01H 12H 22H 22H 22H 23H"), logs + 1);
        int n = 0;
        while (f.rd<std::int32_t>(f.d.game_mode) == 4 && n < 20) {
            freeze(f, 0);
            ++n;
        }
    }
    CHECK_EQ(f.count_logs("cancelled"), 0);
    CHECK_EQ(f.freeze_inits, 3);
    // Held 3 ticks, let go: cancelled on the release (the gate asks again while the press is in its
    // window, and the request ends once it is out), no freeze.
    for (int i = 0; i < 30; ++i) {
        play(f, 0);
    }
    play(f, 0x10);
    play(f, 0x10);
    play(f, 0x10);
    for (int i = 0; i < 15; ++i) {
        play(f, 0);
    }
    CHECK_EQ(f.freeze_inits, 3);
    CHECK_EQ(f.count_logs("cancelled: Triangle let go"), 1);
    CHECK(f.traced(" 01H 12H 22H 21-"));

    // A tap in the 8 ticks after a freeze: the gate does not open, the game's own double tap gets it.
    hold_until_freeze(f);
    while (f.rd<std::int32_t>(f.d.game_mode) == 4) {
        freeze(f, 0);
    }
    play(f, 0x10);
    CHECK(f.logged("fewer than 8 ticks since the game mode changed"));
    CHECK_EQ(f.rd<std::int16_t>(f.d.hero + gcw::kHeroTapTimer), 20);
}

// The double tap's logs: a swap (from, to, its timers), and a second release that does not swap
// and why.
void test_double_tap_logs() {
    Fake f;
    f.opener = true;
    f.rac1_timers = true;
    set_up(f, true, true);
    for (int i = 0; i < 30; ++i) {
        play(f, 0);
    }
    play(f, 0x10);
    play(f, 0);
    play(f, 0x10);
    play(f, 0);
    CHECK_EQ(f.swaps, 1);
    CHECK(f.logged("double tap: swap from item 0x10 to 0x11 (the quick swap item; tap timer 20, window 19 before)"));
    // The quick swap item already in hand: no swap, and why.
    f.wr<std::int32_t>(f.d.hero + gcw::kHeroItem, 0x11);
    for (int i = 0; i < 30; ++i) {
        play(f, 0);
    }
    play(f, 0x10);
    play(f, 0);
    play(f, 0x10);
    play(f, 0);
    CHECK_EQ(f.swaps, 1);
    CHECK(f.logged("no swap: the quick swap item is in hand (item 0x11, quick swap 0x11"));
}

// Weapons picked with the wheel go through Ratchet & Clank's own change of item (the pending item,
// hero_switch_item's bookkeeping), so the double tap swaps between the last two: A, then B, picked;
// a double tap goes back to A, another to B.
void test_wheel_picks_then_double_tap() {
    Fake f;
    f.opener = true;
    f.rac1_timers = true;
    f.rac1_switch = true;
    set_up(f, true, true);
    f.wr<std::int32_t>(f.d.hero + gcw::kHeroItem, 0x10);
    f.wr<std::int32_t>(f.d.hero + gcw::kHeroRequestedItem, 0x10);
    f.wr<std::int32_t>(f.d.last_weapon, 0x10);
    f.wr<std::int32_t>(f.d.quick_swap_item, 0);
    auto pick = [&](float angle) {
        for (int i = 0; i < 30; ++i) {
            play(f, 0);
        }
        hold_until_freeze(f);
        float x, y;
        stick_at(angle, x, y);
        for (int i = 0; i < 6; ++i) {
            freeze(f, 0x10, i < 2 ? 0.0f : x, i < 2 ? 0.0f : y);
        }
        int n = 0;
        while (f.rd<std::int32_t>(f.d.game_mode) == 4 && n < 20) {
            freeze(f, 0);
            ++n;
        }
        for (int i = 0; i < 30; ++i) {
            play(f, 0);
        }
    };
    auto double_tap = [&] {
        for (int i = 0; i < 30; ++i) {
            play(f, 0);
        }
        play(f, 0x10);
        play(f, 0);
        play(f, 0x10);
        play(f, 0);
        for (int i = 0; i < 30; ++i) {
            play(f, 0);
        }
    };
    pick(-kPi / 4);  // slot 1: item 0x11 (A)
    CHECK_EQ(f.rd<std::int32_t>(f.d.hero + gcw::kHeroItem), 0x11);
    pick(0.0f);  // slot 2: item 0x12 (B)
    CHECK_EQ(f.rd<std::int32_t>(f.d.hero + gcw::kHeroItem), 0x12);
    CHECK_EQ(f.rd<std::int32_t>(f.d.quick_swap_item), 0x11);
    CHECK_EQ(f.rd<std::int32_t>(f.d.last_weapon), 0x12);
    double_tap();
    CHECK_EQ(f.swaps, 1);
    CHECK_EQ(f.rd<std::int32_t>(f.d.hero + gcw::kHeroItem), 0x11);
    CHECK(f.logged("double tap: swap from item 0x12 to 0x11 (the quick swap item"));
    double_tap();
    CHECK_EQ(f.swaps, 2);
    CHECK_EQ(f.rd<std::int32_t>(f.d.hero + gcw::kHeroItem), 0x12);
    CHECK(f.logged("double tap: swap from item 0x11 to 0x12 (the quick swap item"));
    CHECK_EQ(f.freeze_inits, 2);
}

// A tick of whichever mode the game is in.
void tick(Fake& f, std::uint32_t held) {
    if (f.rd<std::int32_t>(f.d.game_mode) == 4) {
        freeze(f, held);
    } else {
        play(f, held);
    }
}

// A natural double tap with "Pause while open" on: two presses of `hold` ticks, `gap` ticks apart.
// Going Commando's mode 7 runs the double tap's timers too (its level programs' UpdateModalState
// calls hero_update_button_hold_timers where rac2-decomp's C calls stub_empty_20), so a second press
// long enough to open the paused quick select still swaps as it is let go.
void test_natural_double_tap() {
    for (int hold : {2, 3, 4, 5, 6}) {
        for (int gap : {3, 5, 7}) {
            Fake f;
            f.opener = true;
            f.rac1_timers = true;
            f.rac1_switch = true;
            set_up(f, true, true);
            f.wr<std::int32_t>(f.d.hero + gcw::kHeroItem, 0x12);
            f.wr<std::int32_t>(f.d.hero + gcw::kHeroRequestedItem, 0x12);
            f.wr<std::int32_t>(f.d.last_weapon, 0x12);
            f.wr<std::int32_t>(f.d.quick_swap_item, 0x11);
            for (int i = 0; i < 30; ++i) {
                play(f, 0);
            }
            for (int i = 0; i < hold; ++i) tick(f, 0x10);
            for (int i = 0; i < gap; ++i) tick(f, 0);
            for (int i = 0; i < hold; ++i) tick(f, 0x10);
            for (int i = 0; i < 40; ++i) tick(f, 0);
            if (f.rd<std::int32_t>(f.d.hero + gcw::kHeroItem) != 0x11) {
                std::fprintf(stderr, "natural double tap: hold %d, gap %d: item 0x%X, freezes %d\n", hold, gap,
                             f.rd<std::int32_t>(f.d.hero + gcw::kHeroItem), f.freeze_inits);
            }
            CHECK_EQ(f.rd<std::int32_t>(f.d.hero + gcw::kHeroItem), 0x11);
            CHECK_EQ(f.rd<std::int32_t>(f.d.game_mode), 0);
        }
    }
}

// "Hold before pausing": the countdown's start only. "A little longer" (the default, 8 frames):
// a 6-tick tap never pauses, a natural double tap swaps with no pause, a hold opens at tick 9;
// "Longer" (12) at tick 13; "Going Commando" (4) at tick 5; PAL scaled (8 -> 7: tick 8).
void test_open_delay_setting() {
    struct Case {
        bool ntsc;
        int frames;
        int opens_at;
    };
    for (const Case c : {Case{true, gc::kOpenDelayLittleLonger, 9}, Case{true, gc::kOpenDelayLonger, 13},
                         Case{true, gc::kGcOpenDelay, 5}, Case{false, gc::kOpenDelayLittleLonger, 8}}) {
        Fake f;
        f.opener = true;
        f.rac1_timers = true;
        f.rac1_switch = true;
        set_up(f, c.ntsc, true);
        gc::shim().set_options(true, true, c.frames);
        f.wr<std::int32_t>(f.d.hero + gcw::kHeroItem, 0x12);
        f.wr<std::int32_t>(f.d.hero + gcw::kHeroRequestedItem, 0x12);
        f.wr<std::int32_t>(f.d.last_weapon, 0x12);
        f.wr<std::int32_t>(f.d.quick_swap_item, 0x11);
        for (int i = 0; i < 30; ++i) {
            play(f, 0);
        }
        CHECK_EQ(hold_until_freeze(f) - 1, c.opens_at);
        while (f.rd<std::int32_t>(f.d.game_mode) == 4) {
            freeze(f, 0);
        }
        if (c.frames != gc::kOpenDelayLittleLonger || !c.ntsc) {
            continue;
        }
        // A 6-tick tap: no pause.
        for (int i = 0; i < 30; ++i) play(f, 0);
        const int freezes = f.freeze_inits;
        for (int i = 0; i < 6; ++i) tick(f, 0x10);
        for (int i = 0; i < 30; ++i) tick(f, 0);
        CHECK_EQ(f.freeze_inits, freezes);
        // A double tap of 6-tick presses, 5 apart: the swap, no pause.
        for (int i = 0; i < 6; ++i) tick(f, 0x10);
        for (int i = 0; i < 5; ++i) tick(f, 0);
        for (int i = 0; i < 6; ++i) tick(f, 0x10);
        for (int i = 0; i < 30; ++i) tick(f, 0);
        CHECK_EQ(f.freeze_inits, freezes);
        CHECK_EQ(f.rd<std::int32_t>(f.d.hero + gcw::kHeroItem), 0x11);
        CHECK_EQ(f.swaps, 1);
    }
}

void test_quick_swap() {
    Fake f;
    set_up(f, false, true);
    gc::Shim& s = gc::shim();
    auto timers = [&] {
        gc::CopyScope scope(s);
        hero_update_button_hold_timers();
    };
    const std::uint32_t h = f.d.hero;
    // First press: the tap timer (20 frames: 17 ticks on PAL).
    f.poll(0x10);
    timers();
    // (Both timers count down first, then the press sets the tap timer.)
    CHECK_EQ(f.rd<std::int16_t>(h + gcw::kHeroTapTimer), 17);
    // Released: the window opens.
    f.poll(0);
    timers();
    CHECK_EQ(f.rd<std::int16_t>(h + gcw::kHeroTapWindow), 17);
    // Pressed and released again in the window: the quick swap item pending, the timers cleared,
    // the swap counted.
    f.poll(0x10);
    timers();
    f.poll(0);
    timers();
    CHECK_EQ(f.rd<std::int32_t>(h + gcw::kHeroPendingItem), 0x11);
    CHECK_EQ(f.rd<std::int16_t>(h + gcw::kHeroTapTimer), 0);
    CHECK_EQ(f.rd<std::int16_t>(h + gcw::kHeroTapWindow), 0);
    CHECK_EQ(f.rd<std::uint16_t>(f.d.usage_stats + gcw::kUsageStride * gcw::kUsageQuickSwap), 1);
    // With the wrench in hand: the last weapon.
    f.wr<std::int32_t>(h + gcw::kHeroItem, 8);
    f.wr<std::int32_t>(h + gcw::kHeroPendingItem, 0);
    f.poll(0x10);
    timers();
    f.poll(0);
    timers();
    f.poll(0x10);
    timers();
    f.poll(0);
    timers();
    CHECK_EQ(f.rd<std::int32_t>(h + gcw::kHeroPendingItem), 0x13);
    // The window runs out: no swap.
    f.wr<std::int32_t>(h + gcw::kHeroPendingItem, 0);
    f.poll(0x10);
    timers();
    f.poll(0);
    timers();
    for (int i = 0; i < 40; ++i) {
        f.poll(0);
        timers();
    }
    f.poll(0x10);
    timers();
    f.poll(0);
    timers();
    CHECK_EQ(f.rd<std::int32_t>(h + gcw::kHeroPendingItem), 0);
}

}  // namespace

int main() {
    test_frames_and_ticks();
    test_pad_history_and_press_window();
    test_selection_math();
    test_dpad_neighbours();
    test_fades_and_commit();
    test_draw();
    test_eqs_paused_page_commit();
    test_paused_path();
    test_gate_conditions();
    test_relocated_game_mode();
    test_level_callbacks();
    test_taps();
    test_hold_timing();
    test_double_tap_logs();
    test_wheel_picks_then_double_tap();
    test_natural_double_tap();
    test_open_delay_setting();
    test_unavailable_paused_open();
    test_watchdog_update_bypassed();
    test_watchdog_record_never_opens();
    test_quick_swap();
    std::printf("%d checks, %d failed\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
