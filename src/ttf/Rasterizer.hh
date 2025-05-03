#pragma once

#include "ttf/types.hh"

#include "Image.hh"

#include "adt/Pair.hh"

namespace ttf
{

struct Font;

struct Rasterizer
{
    static constexpr adt::f32 X_STEP = 0.55f; /* x-axis spacing between glyphs */

    /* */

    Image m_altas {};
    adt::Map<adt::u32, adt::Pair<adt::i16, adt::i16>, adt::hash::dumbFunc> m_mapCodeToUV {};
    adt::f32 m_scale {};

    /* */

    void destroy(adt::IAllocator* pAlloc);
    void rasterizeAscii(adt::IAllocator* pAlloc, Font* pFont, adt::f32 scale); /* NOTE: uses app::gtl_scratch */

protected:
    void rasterizeGlyph(const Font& pFont, const Glyph& pGlyph, int xOff, int yOff); /* NOTE: uses app::gtl_scratch */
};

} /* namespace ttf */
