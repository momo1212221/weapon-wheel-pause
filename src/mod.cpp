// SPDX-License-Identifier: GPL-3.0-or-later
//
// Weapon Wheel Pause (originally by Moritz Wallinger and glawks): Going Commando's weapon quick select in Ratchet & Clank, running
// Going Commando's own code.
//
// The quick select is Going Commando's, copied (src/gc/*.c, from rac2-decomp): its update (the
// stick's sectors, the d-pad's neighbours, the click, the commit as Triangle is let go), its draw
// (the selected weapon's ammo and name; Ratchet & Clank's own ring), its init, and its pause: Triangle
// pressed in play opens Going Commando's game mode 4 with modal mode 7 (the gate, then
// UpdateModalState's case 7 and gfx_render_frame_extra_layers), which here runs in Ratchet & Clank's
// game mode 4, the freeze mode, with a freeze kind of the mod's own. The translation layer
// (src/gc/rac1_shim.hpp) runs it on Ratchet & Clank's memory and functions.
//
// Six hook points:
//   rac1.level.update_play          game mode 0's tick: Going Commando's gate before it; when the
//                                   gate opens the quick select, the tick is skipped (the world held)
//   rac1.freeze.update              the freeze mode's tick: the mod's kind runs UpdateModalState
//                                   (case 7), then the game's (the sound)
//   rac1.freeze.draw                the freeze mode's picture: the mod's kind draws Going Commando's
//                                   (the world and its HUD, no dialog)
//   rac1.hud.quick_select.init      the quick select's HUD record: Going Commando's init,
//   rac1.hud.quick_select.update    update and draw, paused or not ("Pause while open" off: Ratchet &
//   rac1.hud.quick_select.draw      Clank's own Triangle opens it, Going Commando's code runs it)
// The first three need OpenRAC with mod interface 1.39 and those hook points (sdk.lock's commit).
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#include "openrac/games/rac1.hpp"
#include "openrac/mod.hpp"

#include "gc/rac1_shim.hpp"
#include "layout.hpp"

namespace {

namespace gcw = gc_weapon_wheel;
namespace gc = gc_weapon_wheel::gc;
namespace rac1 = openrac::rac1;

openrac::Mod* g_mod = nullptr;

gc::Shim& layer() { return gc::shim(); }

// "Hold before pausing": the HUD-open request's countdown, Going Commando's 4 frames or longer.
int open_delay_frames(const std::string& choice) {
    if (choice == "going_commando") {
        return gc::kGcOpenDelay;
    }
    if (choice == "longer") {
        return gc::kOpenDelayLonger;
    }
    return gc::kOpenDelayLittleLonger;
}

void read_settings(openrac::Mod& mod) {
    layer().set_options(mod.setting<bool>("pause"), mod.setting<bool>("click"),
                        open_delay_frames(mod.setting<std::string>("open_delay")));
}

void connect() { layer().connect(g_mod->loaded_level()); }

// The first time each hook point runs, in the log.
void first(const char* hook) {
    layer().log_once(std::string("hook ") + hook, std::string("first call of ") + hook);
}

template <class Fn>
Fn optional_function(openrac::Mod& mod, const char* name) {
    return mod.function<Fn>(name);
}

void connect_game(openrac::Mod& mod, bool ntsc) {
    gc::Game& g = layer().game;
    openrac::Mod* m = &mod;
    g.memory = gc::Memory{mod.memory().base()};
    g.disc = gcw::disc_for(ntsc);
    const gcw::FunctionNames n = gcw::function_names(ntsc);

    g.relocate = [m](std::uint32_t a) { return m->relocate_level(0, a); };
    g.relocate_exe = [m](std::uint32_t a) { return m->relocate(a); };
    // A level's copy of a function, at its place from the decomps' catalogues (layout.hpp): taken
    // only when the disc's function table has the function of that name there.
    g.code_matches = [m](std::uint32_t address, const char* name) {
        using Any = void (*)();
        const Any at = m->function_at<Any>(address);
        return at != nullptr && at == m->function<Any>(name);
    };
    using V = void (*)();
    using VI = void (*)(int);
    using II = int (*)(int);
    const auto freeze_init = mod.required_function<void (*)(unsigned int, int, int)>(n.mode_freeze_init);
    g.mode_freeze_init = [freeze_init](int kind, int a, int b) { freeze_init(static_cast<unsigned int>(kind), a, b); };
    g.update_hud_elements = mod.required_function<II>(n.update_hud_elements);
    if (const auto help = optional_function<V>(mod, n.help_update)) {
        g.help_update = help;
    }
    g.music_unpause = mod.required_function<V>(n.music_unpause);
    g.music_pause = mod.required_function<VI>(n.music_pause);
    g.snd_pause_group = mod.required_function<VI>(n.snd_pause_group);
    g.snd_continue_group = mod.required_function<VI>(n.snd_continue_group);
    const auto flush = mod.required_function<int (*)()>(n.snd_flush);
    g.snd_flush = [flush] { flush(); };
    g.draw_hud_sprite = mod.required_function<void (*)(int, int, int, int, int, int)>(n.draw_hud_sprite);
    g.get_icon_frame = mod.required_function<int (*)(int, int)>(n.get_icon_frame);
    const auto frame = mod.required_function<void (*)(std::uint32_t, int, int, int, unsigned int, int)>(n.hud_frame);
    g.hud_frame = [frame](std::uint32_t rec, int f, int x, int y, std::uint32_t scale, int alpha) {
        frame(rec, f, x, y, scale, alpha);
    };
    // FontPrintCenter takes the text's game address: the text goes on the game's stack for the call.
    const auto print = mod.required_function<int (*)(int, int, unsigned long long, int, int)>(n.font_print_center);
    g.font_print_center = [m, print](int x, int y, std::uint32_t colour, const char* text, int length) {
        const std::size_t size = std::strlen(text) + 1;
        const openrac::StackFrame frame(*m, static_cast<std::uint32_t>(size));
        std::memcpy(m->memory().pointer(frame.address()), text, size);
        print(x, y, colour, static_cast<int>(frame.address()), length);
    };
    const auto msg = mod.required_function<int (*)(int)>(n.msg_string);
    g.msg_string = [msg](int id) { return static_cast<std::uint32_t>(msg(id)); };
    const auto tween = mod.required_function<unsigned long long (*)(unsigned int, unsigned int, float)>(n.fast_tween_color);
    g.fast_tween_color = [tween](std::uint32_t a, std::uint32_t b, float t) { return static_cast<std::uint32_t>(tween(a, b, t)); };
    // hud_screen_position writes through pointers to its x and y: they go on the game's stack.
    const auto position = mod.required_function<int (*)(std::uint32_t, std::uint32_t, std::uint32_t)>(n.hud_screen_position);
    g.hud_screen_position = [m, position](std::uint32_t rec, int& x, int& y) {
        const openrac::StackFrame frame(*m, 8);
        const openrac::Memory memory = m->memory();
        memory.write<std::int32_t>(frame.address(), x);
        memory.write<std::int32_t>(frame.address() + 4, y);
        position(rec, frame.address(), frame.address() + 4);
        x = memory.read<std::int32_t>(frame.address());
        y = memory.read<std::int32_t>(frame.address() + 4);
    };
    g.show_health_hud = mod.required_function<int (*)()>(n.show_health_hud);
    g.hud_keep_visible = mod.required_function<int (*)(int, int)>(n.hud_keep_visible);
    const auto queue = mod.required_function<int (*)(int, int, int, int, int, int, int)>(n.queue_animation_update);
    g.queue_animation_update = [queue](int key, int a, std::uint32_t init, std::uint32_t update, std::uint32_t draw,
                                       std::uint32_t data, int b) {
        return queue(key, a, static_cast<int>(init), static_cast<int>(update), static_cast<int>(draw),
                     static_cast<int>(data), b);
    };
    g.remove_hud_item = mod.required_function<II>(n.remove_hud_item);
    g.append_gif_transfer_packet = mod.required_function<V>(n.append_gif_transfer_packet);
    g.draw_debug_profiler = mod.required_function<V>(n.draw_debug_profiler);
    const auto bank = mod.required_function<int (*)(int, int, int)>(n.play_bank_sound);
    g.play_click = [bank] { bank(0, 1, 0); };

    // The math: Ratchet & Clank's own where its build has them by these names.
    if (const auto f = optional_function<float (*)(float)>(mod, n.square_root)) g.square_root = f;
    if (const auto f = optional_function<float (*)(float, float)>(mod, n.fast_arc_tan)) g.fast_arc_tan = f;
    if (const auto f = optional_function<float (*)(float, float)>(mod, n.diff_rots)) g.diff_rots = f;
    if (const auto f = optional_function<float (*)(float, float)>(mod, n.sub_rots)) g.sub_rots = f;
    if (const auto f = optional_function<float (*)(float)>(mod, n.normalize_angle)) g.normalize_angle = f;
    if (const auto f = optional_function<float (*)(float, float)>(mod, n.add_rots)) g.add_rots = f;
    if (const auto f = optional_function<float (*)(float)>(mod, n.sin)) g.sin = f;
    if (const auto f = optional_function<float (*)(float)>(mod, n.cos)) g.cos = f;
    if (const auto f = optional_function<float (*)(int)>(mod, n.int_to_float)) g.int_to_float = f;
    if (const auto f = optional_function<int (*)(float)>(mod, n.float_to_int)) g.float_to_int = f;

    g.log = [m](const char* what) { m->warn(std::string("Weapon Wheel Pause: ") + what); };
    g.info = [m](const char* what) { m->info(std::string("Weapon Wheel Pause: ") + what); };
}

void load(openrac::Mod& mod) {
    if (mod.game() != "rac1") {
        throw openrac::Error("this mod is for Ratchet & Clank");
    }
    const bool ntsc = mod.version() == "ntsc";
    if (!ntsc && mod.version() != "pal") {
        throw openrac::Error("this mod knows the PAL and NTSC-U discs only");
    }
    g_mod = &mod;
    connect_game(mod, ntsc);
    read_settings(mod);

    mod.hook<rac1::Level_UpdatePlay>([](auto original) {
        connect();
        first("rac1.level.update_play");
        layer().update_play([&] { original(); });
    });
    mod.hook<rac1::Freeze_Update>([](auto original) {
        connect();
        first("rac1.freeze.update");
        layer().freeze_update([&] { original(); });
    });
    mod.hook<rac1::Freeze_Draw>([](auto original) {
        connect();
        first("rac1.freeze.draw");
        layer().freeze_draw([&] { original(); });
    });
    mod.hook<rac1::Hud_QuickSelect_Init>([](auto, std::uint32_t record) {
        connect();
        first("rac1.hud.quick_select.init");
        layer().selector_init(record);
    });
    mod.hook<rac1::Hud_QuickSelect_Update>([](auto, std::uint32_t record) {
        connect();
        first("rac1.hud.quick_select.update");
        layer().selector_update(record);
    });
    mod.hook<rac1::Hud_QuickSelect_Draw>([](auto, std::uint32_t record) -> std::int32_t {
        connect();
        first("rac1.hud.quick_select.draw");
        return layer().selector_draw(record);
    });

    if (mod.has_settings_set()) {
        mod.on_settings([] { read_settings(*g_mod); });
    }
    mod.info("Weapon Wheel Pause: Going Commando's quick select on Triangle (its own code)");
}

}  // namespace

OPENRAC_MOD_CPP("weapon-wheel-pause", "2.0.0", load)
