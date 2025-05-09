#pragma once

#include "types.hh"

#include "adt/bin.hh"

namespace ttf
{

struct Parser
{
    adt::IAllocator* m_pAlloc {};
    adt::StringView m_svFile {};
    adt::bin::Reader m_bin {};
    TableDirectory m_tableDirectory {};
    Head m_head {};
    Cmap m_cmap {};
    CmapFormat4 m_cmapF4 {};
    adt::Map<adt::u32, Glyph> m_mapOffsetToGlyph {};
    bool m_bParsed {};

    /* */
    
    explicit operator bool() const { return m_bParsed; }

    /* */

    Parser() = default;
    Parser(adt::IAllocator* pAlloc, adt::StringView svFile)
        : m_pAlloc(pAlloc), m_svFile(svFile), m_bin(m_svFile)
    {
        m_bParsed = parse();
    }

    /* */

    Glyph* readGlyph(adt::u32 codePoint);
    void printGlyphDBG(const Glyph& g, bool bNormalize = false);
    void destroy();

    /* */

private:
    bool parse();
    bool parse2();
    adt::MapResult<adt::StringView, TableRecord> getTable(adt::StringView sTableTag);
    void readHeadTable();
    void readCmapTable();
    void readCmap(adt::u32 offset);
    void readCmapFormat4();
    adt::u32 getGlyphOffset(adt::u32 idx);
    adt::u32 getGlyphIdx(adt::u16 code);
    FWord readFWord();
    void readCompoundGlyph(Glyph* g);
    void readSimpleGlyph(Glyph* g);
};

} /* namespace ttf */
