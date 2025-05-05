#pragma once

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

    /* */

    union
    {
        const char* ntsText;
        adt::f64 updateRateMS;
    };
    TYPE eType {};

    /* */

    static StatusEntry makeText(const char* _ntsText) { return {.ntsText = _ntsText, .eType = TYPE::TEXT, }; }
    static StatusEntry makeDateTime(const char* ntsDateTimeFormat) { return {.ntsText = ntsDateTimeFormat, .eType = TYPE::DATE_TIME, }; }
    static StatusEntry makeKeyboardLayout() { return {.ntsText {}, .eType = TYPE::KEYBOARD_LAYOUT}; }
    static StatusEntry makeFileWatch(adt::f64 _updateRateMS) { return {.updateRateMS = _updateRateMS, .eType = TYPE::FILE_WATCH}; }
};

inline const StatusEntry inl_aStatusEntries[] {
    StatusEntry::makeDateTime("%Y-%m-%d %I:%M%p"), /* `man strftime` */
    StatusEntry::makeKeyboardLayout(),
};

} /* namespace config */
