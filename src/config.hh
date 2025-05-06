#pragma once

#include "adt/StringDecl.hh"
#include "adt/print.hh"
#include "adt/types.hh"

namespace config
{

using String64 = adt::StringFixed<64>;
using PfnFormatString = String64 (*)(const char* ntsFormat);

struct StatusEntry
{
    enum class TYPE : adt::u8
    {
        TEXT,
        DATE_TIME,
        KEYBOARD_LAYOUT,
        FILE_WATCH,
    };

    /* */

    const char* nts {};
    adt::f64 updateRateMS {};
    const char* ntsFormat {};
    PfnFormatString pfnFormatString {};

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

    static StatusEntry
    makeKeyboardLayout()
    {
        return {.eType = TYPE::KEYBOARD_LAYOUT };
    }

    static StatusEntry
    makeFileWatch(const char* ntsFilePath, adt::f64 updateRateMS, PfnFormatString pfnFormatString = nullptr)
    {
        return {
            .nts = ntsFilePath,
            .updateRateMS = updateRateMS,
            .pfnFormatString = pfnFormatString,
            .eType = TYPE::FILE_WATCH
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

inline StatusEntry inl_aStatusEntries[] {
    StatusEntry::makeFileWatch("/sys/class/drm/card1/device/hwmon/hwmon3/temp2_input", 3000.0, formatGpuTempC),
    StatusEntry::makeFileWatch("/sys/class/drm/card1/device/hwmon/hwmon3/power1_input", 3000.0, formatGpuPower),
    StatusEntry::makeDateTime("%Y-%m-%d %I:%M%p", 1000.0*30), /* `man strftime` */
    StatusEntry::makeKeyboardLayout(),
};

} /* namespace config */
