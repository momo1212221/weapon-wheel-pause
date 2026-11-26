/* SPDX-License-Identifier: GPL-3.0-only
 *
 * COPIED from Going Commando's decompilation: OpenRAC/rac2-decomp
 * (https://github.com/OpenRAC/rac2-decomp, GNU GPL version 3) at commit
 * 4ea93ceaf43aec055e19211d36512e847860bac2, file src/levels/shared/L00_0032EF10.c,
 * lines 1-32 (all). Code of that project from before its relicensing (d46a3539, 2026-10-11) is also
 * under its MIT notice: THIRD_PARTY_NOTICES.md.
 *
 * PadPressedWithin (USA v1.01 func_L00_0032EF10): a button pressed within the pad's last n polls.
 * Ratchet & Clank's pad keeps the same history at the same offsets (PadState: hist_idx 0x18E,
 * hist_count 0x190, hist_btn[30] 0x1E0, include/structs/pad.h).
 *
 * The copied lines are unchanged. None is changed. Every name the copied lines use that is not
 * defined in src/gc/ is the translation layer's (gc/shim/common.h, gc/rac1_shim.cpp): Going
 * Commando's globals and functions in Ratchet & Clank's terms.
 */
#include "common.h"

typedef struct {
    char reserved000[0x18E];
    s16 head;
    s32 count;
    char reserved194[0x1E0 - 0x194];
    s32 ring[30];
} History;

extern History g_pad_state;

/* walks the 30-entry history ring back from its head (at most arg1 entries) and returns the
   first entry's mask bits, storing how far back it was */
s32 PadPressedWithin(u32 mask, s32 arg1, s32 *out)
{
    s32 n = g_pad_state.count;
    s32 i;

    if (n >= arg1)
        n = arg1;
    for (i = 1; i < n + 1; i++) {
        s32 *p = &g_pad_state.ring[(g_pad_state.head - i + 30) % 30];

        if ((*p & mask) != 0) {
            if (out != 0)
                *out = i - 1;
            return *p & mask;
        }
    }
    return 0;
}
