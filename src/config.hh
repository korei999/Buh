#pragma once

#include "adt/StringDecl.hh"
#include "adt/print.hh"
#include "adt/types.hh"

namespace config
{

struct StatusEntry
{
    enum class TYPE : adt::u8
    {
        TEXT,
        DATE_TIME,
        KEYBOARD_LAYOUT,
        FILE_WATCH,
    };

    using PfnFormatString = adt::StringFixed<64> (*)(const char* ntsFormat);

    /* */

    const char* nts {};
    adt::f64 updateRateMS {};
    const char* ntsFormat {};
    PfnFormatString pfnFormatString {};
    TYPE eType {};

    /* */

    static StatusEntry
    makeText(const char* _ntsText)
    {
        return {
            .nts = _ntsText,
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
        return {
            .nts {},
            .updateRateMS = -1.0,
            .eType = TYPE::KEYBOARD_LAYOUT
        };
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

inline adt::StringFixed<64>
formatGpuPower(const char* fmt)
{
    adt::StringFixed<64> sfRet {};

    long long num = atoll(fmt);
    adt::print::toSpan({sfRet.data(), 64}, "(GPU) {}W", num / 1000000);

    return sfRet;
}

inline const StatusEntry inl_aStatusEntries[] {
    StatusEntry::makeFileWatch("/sys/class/drm/card1/device/hwmon/hwmon3/power1_input", 5000.0, formatGpuPower),
    StatusEntry::makeDateTime("%Y-%m-%d %I:%M%p", 1000.0*30), /* `man strftime` */
    StatusEntry::makeKeyboardLayout(),
};

} /* namespace config */
