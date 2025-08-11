#pragma once

namespace battery
{

enum class STATUS : adt::u8 { ERROR, DISCHARCHING, CHARGING, NOT_CHARCHING, FULL };

struct Report
{
    int cap {};
    STATUS eStatus {};

    /* */

    static Report read(const char* ntsPath, adt::IArena* pArena);
};

} /* namespace battery */

namespace adt::print
{

inline isize
format(Context ctx, FormatArgs fmtArgs, const battery::STATUS x)
{
    constexpr StringView map[] {
        "Error", "Discharching", "Charging", "Not charching", "Full"
    };
    return format(ctx, fmtArgs, map[int(x)]);
}

} /* namespace adt::print */
