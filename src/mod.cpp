#include "openrac/mod.hpp"
#include "openrac/games/rac1.hpp"

namespace {

bool wheel_active = false;
bool wheel_seen_this_tick = false;
bool world_gate = false;

int freeze_moby_updates(void*, int, int phase) {
    return wheel_active && phase == OPENRAC_MOBY_BEFORE ? 1 : 0;
}

void load(openrac::Mod& mod) {
    if (mod.game() != "rac1") {
        throw openrac::Error("Weapon Wheel Pause is for Ratchet & Clank");
    }

    // The selector callback exists only while the in-game quick-select wheel is alive.
    // Mark it before running the game's own code so the wheel remains fully functional.
    mod.hook<openrac::rac1::Hud_QuickSelect_Update>([](auto original, std::uint32_t selector) {
        wheel_active = true;
        wheel_seen_this_tick = true;
        original(selector);
    });

    // Ratchet is not a normal moby update, so freeze his whole gameplay update separately.
    mod.hook<openrac::rac1::Hero_Update>([](auto original) {
        if (!wheel_active) {
            original();
        }
    });

    // Freeze enemies, projectiles, crates, doors and other mobys when no other mod owns the
    // world-update gate. Multiplayer currently owns this gate, so failure is a supported fallback.
    try {
        mod.on_moby_update(freeze_moby_updates);
        world_gate = true;
    } catch (const openrac::Error&) {
        world_gate = false;
    }

    // If the wheel callback was not seen during a tick, it has closed. Keeping the state for one
    // extra tick avoids racing the selector's final fade/removal frame.
    mod.on_frame([] {
        if (!wheel_seen_this_tick) {
            wheel_active = false;
        }
        wheel_seen_this_tick = false;
    });

    mod.info(world_gate ? "weapon wheel pauses Ratchet and world updates"
                        : "weapon wheel pauses Ratchet; world gate is owned by another mod");
}

}  // namespace

OPENRAC_MOD_CPP("weapon-wheel-pause", "0.1.0", load)
