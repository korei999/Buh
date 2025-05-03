#pragma once

#include "adt/IAllocator.hh"
#include "adt/Span2D.hh"
#include "adt/types.hh"

union ImagePixelRGBA
{
    struct { adt::u8 r, g, b, a; };
    adt::u32 data;
    adt::i32 iData;
};

union ImagePixelRGB
{
    struct { adt::u8 r, g, b; };
    adt::u8 aData[3];
};

struct Image
{
    enum class TYPE : adt::u8 { RGBA, RGB, MONO };

    /* */

    union
    {
        ImagePixelRGBA* pRGBA;
        ImagePixelRGB* pRGB;
        adt::u8* pMono;
    } m_uData {};
    adt::i16 m_width {};
    adt::i16 m_height {};
    TYPE m_eType {};

    /* */

    [[nodiscard]] Image cloneToRGBA(adt::IAllocator* pAlloc);
    void swapRedBlue();
    void flipVertically(adt::IAllocator* pAlloc);

    adt::Span2D<ImagePixelRGBA>
    spanRGBA()
    {
        ADT_ASSERT(m_eType == TYPE::RGBA, " ");
        return {m_uData.pRGBA, m_width, m_height, m_width};
    }

    const adt::Span2D<ImagePixelRGBA>
    spanRGBA() const
    {
        ADT_ASSERT(m_eType == TYPE::RGBA, " ");
        return {m_uData.pRGBA, m_width, m_height, m_width};
    }

    adt::Span2D<ImagePixelRGB>
    spanRGB()
    {
        ADT_ASSERT(m_eType == TYPE::RGB, " ");
        return {m_uData.pRGB, m_width, m_height, m_width};
    }

    const adt::Span2D<ImagePixelRGB>
    spanRGB() const
    {
        ADT_ASSERT(m_eType == TYPE::RGB, " ");
        return {m_uData.pRGB, m_width, m_height, m_width};
    }

    adt::Span2D<adt::u8>
    spanMono()
    {
        ADT_ASSERT(m_eType == TYPE::MONO, " ");
        return {m_uData.pMono, m_width, m_height, m_width};
    }

    adt::Span2D<adt::u8>
    spanMono() const
    {
        ADT_ASSERT(m_eType == TYPE::MONO, " ");
        return {m_uData.pMono, m_width, m_height, m_width};
    }
};
