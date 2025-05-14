#pragma once

#include "adt/String.hh"
#include "ColorScheme.hh"
#include "battery.hh"

namespace config
{

using String64 = adt::StringFixed<64>;
using PfnFormatString = String64 (*)(const char* ntsFormat);
using PfnFormatBattery = String64 (*)(const battery::Report report);

struct StatusEntry
{
    enum class TYPE : adt::u8
    {
        TEXT,
        DATE_TIME,
        KEYBOARD_LAYOUT,
        FILE_WATCH,
        BATTERY,
    };

    /* */

    const char* nts {};
    adt::f64 updateRateMS {};
    const char* ntsFormat {};

    union
    {
        PfnFormatString pfnFormatString;
        PfnFormatBattery pfnFormatBattery;
    } func {};

    String64 sfHolder {};
    adt::f64 lastUpdateTimeMS {};

    TYPE eType {};

    /* */

    static StatusEntry
    makeText(const char* ntsText)
    {
        return {
            .nts = ntsText,
            .updateRateMS = -1.0,
            .eType = TYPE::TEXT,
        };
    }

    static StatusEntry
    makeDateTime(const char* ntsDateTimeFormat, adt::f64 updateRateMS)
    {
        return {
            .nts = ntsDateTimeFormat,
            .updateRateMS = updateRateMS,
            .eType = TYPE::DATE_TIME,
        };
    }

#ifdef OPT_IPC_KBLAYOUT
    static StatusEntry
    makeKeyboardLayout()
    {
        return {.eType = TYPE::KEYBOARD_LAYOUT };
    }
#endif

    static StatusEntry
    makeFileWatch(const char* ntsFilePath, adt::f64 updateRateMS, PfnFormatString pfnFormatString = nullptr)
    {
        return {
            .nts = ntsFilePath,
            .updateRateMS = updateRateMS,
            .func {.pfnFormatString = pfnFormatString},
            .eType = TYPE::FILE_WATCH
        };
    }

    static StatusEntry
    makeBattery(const char* ntsFilePath, adt::f64 updateRateMS, PfnFormatBattery pfnFormatBattery = nullptr)
    {
        return {
            .nts = ntsFilePath,
            .updateRateMS = updateRateMS,
            .func {.pfnFormatBattery = pfnFormatBattery},
            .eType = TYPE::BATTERY
        };
    }
};

inline String64
formatGpuTempC(const char* fmt)
{
    String64 sfRet {};

    long long num = atoll(fmt);
    adt::print::toSpan({sfRet.data(), String64::CAP},
        "{}C", num / 1000
    );

    return sfRet;
}

inline String64
formatGpuPower(const char* fmt)
{
    String64 sfRet {};

    long long num = atoll(fmt);
    adt::print::toSpan({sfRet.data(), String64::CAP},
        "{}W", num / 1000000
    );

    return sfRet;
}

inline String64
formatBattery(const battery::Report report)
{
    String64 sfRet {};
    adt::print::toSpan({sfRet.data(), String64::CAP},
        "{} {}%", report.eStatus, report.cap
    );

    return sfRet;
}

inline StatusEntry inl_aStatusEntries[] {
    StatusEntry::makeFileWatch("/sys/class/drm/card1/device/hwmon/hwmon3/temp2_input", 3000.0, formatGpuTempC),
    StatusEntry::makeFileWatch("/sys/class/drm/card1/device/hwmon/hwmon3/power1_input", 3000.0, formatGpuPower),
    StatusEntry::makeDateTime("%Y-%m-%d %I:%M%p", 1000.0*30), /* `man strftime` */
    StatusEntry::makeBattery("/sys/class/power_supply/BATT/", 5000.0, formatBattery),
#ifdef OPT_IPC_KBLAYOUT
    StatusEntry::makeKeyboardLayout(),
#endif
};

constexpr ColorScheme aColorSchemes[] {
    {
        .tag       {.fg = 0xffbbbbbb, .bg = 0xff222222},
        .activeTag {.fg = 0xffbbbbbb, .bg = 0xff005577},
        .urgentTag {.fg = 0xff000000, .bg = 0xffff0000},
        .title     {.fg = 0xffbbbbbb, .bg = 0xff005577},
        .status    {.fg = 0xffbbbbbb, .bg = 0xff222222},
    },
    {
        .tag       {.fg = 0xff555555, .bg = 0xff777777},
        .activeTag {.fg = 0xff000000, .bg = 0xff777777},
        .urgentTag {.fg = 0xff000000, .bg = 0xffac4242},
        .title     {.fg = 0xff000000, .bg = 0xff777777},
        .status    {.fg = 0xff000000, .bg = 0xff777777},
    },
};

inline ColorScheme inl_colorScheme = aColorSchemes[0];

} /* namespace config */
