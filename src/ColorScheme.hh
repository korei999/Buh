#pragma once

#include "adt/types.hh"

struct ColorScheme
{
    struct Color
    {
        adt::u32 fg {};
        adt::u32 bg {};
    };

    /* */

    Color tag {};
    Color activeTag {};
    Color urgentTag {};
    Color title {};
    Color status {};
};
