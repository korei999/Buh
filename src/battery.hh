#pragma once

#include "adt/ScratchBuffer.hh"
#include "adt/print.hh"

namespace battery
{

enum class STATUS : adt::u8 { ERROR, DISCHARCHING, CHARGING, NOT_CHARCHING, };

struct Report
{
    int cap {};
    STATUS eStatus {};

    /* */

    static Report read(const char* ntsPath, adt::ScratchBuffer* pScratch);
};

} /* namespace battery */

namespace adt::print
{

inline isize
formatToContext(Context ctx, FormatArgs fmtArgs, const battery::STATUS x)
{
    constexpr StringView map[] {
        "Error", "Discharching", "Charging", "Not charching"
    };
    return formatToContext(ctx, fmtArgs, map[int(x)]);
}

} /* namespace adt::print */
