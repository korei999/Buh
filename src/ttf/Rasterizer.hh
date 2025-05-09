#pragma once

#include "ttf/types.hh"

#include "Image.hh"

#include "adt/Pair.hh"

namespace ttf
{

struct Parser;

struct Rasterizer
{
    static constexpr adt::f32 X_STEP = 0.50f; /* x-axis spacing between glyphs */

    /* */

    Image m_altas {};
    adt::Map<adt::u32, adt::Pair<adt::i16, adt::i16>, adt::hash::dumbFunc> m_mapCodeToUV {};
    adt::f32 m_scale {};
    adt::i16 m_xOffAtlas {};
    adt::i16 m_yOffAtlas {};

    /* */

    void destroy(adt::IAllocator* pAlloc);
    void rasterizeAsciiIntoAltas(adt::IAllocator* pAlloc, Parser* pFont, adt::f32 scale); /* NOTE: uses app::gtl_scratch */

    adt::Span2D<adt::u8> atlasSpan() { return m_altas.spanMono(); }
    const adt::Span2D<adt::u8> atlasSpan() const { return m_altas.spanMono(); }

    adt::MapResult<adt::u32, adt::Pair<adt::i16, adt::i16>> searchGlyphAtlasUV(adt::u32 code) const { return m_mapCodeToUV.search(code); }
    adt::MapResult<adt::u32, adt::Pair<adt::i16, adt::i16>> readGlyph(adt::IAllocator* pAlloc, Parser* pFont, adt::u32 code);

    void rasterizeGlyph(const Parser& pFont, const Glyph& pGlyph, int xOff, int yOff); /* NOTE: uses app::gtl_scratch */

protected:
    int m_nSquares {};
    int m_altasAllocSize {};
};

} /* namespace ttf */
