#pragma once

#include "ttf/types.hh"

#include "Image.hh"

namespace ttf
{

struct Font;

struct Rasterizer
{
    static constexpr adt::f32 X_STEP = 0.50f; /* x-axis spacing between glyphs */

    /* */

    Image m_altas {};
    adt::Map<adt::u32, adt::Pair<adt::i16, adt::i16>, adt::hash::dumbFunc> m_mapCodeToUV {};
    adt::f32 m_scale {};

    /* */

    void destroy(adt::IAllocator* pAlloc);
    void rasterizeAscii(adt::IAllocator* pAlloc, Font* pFont, adt::f32 scale); /* NOTE: uses app::gtl_scratch */

    adt::Span2D<adt::u8> atlasSpan() { return m_altas.spanMono(); }
    const adt::Span2D<adt::u8> atlasSpan() const { return m_altas.spanMono(); }

    adt::MapResult<adt::u32, adt::Pair<adt::i16, adt::i16>> searchGlyph(adt::u32 code) const { return m_mapCodeToUV.search(code); }

    void rasterizeGlyph(const Font& pFont, const Glyph& pGlyph, int xOff, int yOff); /* NOTE: uses app::gtl_scratch */
};

} /* namespace ttf */
