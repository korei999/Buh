#pragma once

#include "ttf/types.hh"
#include "ttf/Parser.hh"

#include "Image.hh"

#include "adt/ThreadPool.hh"

namespace ttf
{

struct Rasterizer
{
    using UVSearch = adt::MapResult<adt::u32, adt::Pair<adt::i16, adt::i16>>;
    static constexpr adt::f32 X_STEP = 0.50f; /* x-axis spacing between glyphs */

    /* */

    Image m_atlas {};
    adt::Map<adt::u32, adt::Pair<adt::i16, adt::i16>, adt::hash::dumbFunc> m_mapCodeToUV {};
    adt::f32 m_scale {};
    adt::i16 m_xOffAtlas {};
    adt::i16 m_yOffAtlas {};

    /* */

    void destroy(adt::IAllocator* pAlloc);
    void rasterizeAscii(adt::IAllocator* pAlloc, Parser* pFont, adt::IThreadPoolWithMemory* pThreadPool, adt::f32 scale);
    UVSearch addOrSearchGlyph(adt::ScratchBuffer* pScratch, adt::IAllocator* pAlloc, Parser* pFont, adt::u32 code);
    void rasterizeGlyph(adt::ScratchBuffer* pScratch, const Parser& pFont, const Glyph& pGlyph, int xOff, int yOff);

    adt::Span2D<adt::u8> atlasSpan() { return m_atlas.spanMono(); }
    const adt::Span2D<adt::u8> atlasSpan() const { return m_atlas.spanMono(); }
    UVSearch searchGlyphAtlasUV(adt::u32 code) const { return m_mapCodeToUV.search(code); }

};

} /* namespace ttf */
