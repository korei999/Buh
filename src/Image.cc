#include "Image.hh"

#include "adt/defer.hh"
#include "adt/utils.hh"
#include "adt/simd.hh"

using namespace adt;

Image
Image::cloneToRGBA(adt::IAllocator* pAlloc)
{
    Image nImg {
        .m_uData {},
        .m_width = m_width,
        .m_height = m_height,
        .m_eType = Image::TYPE::RGBA,
    };

    nImg.m_uData.pRGBA = pAlloc->zallocV<ImagePixelRGBA>(m_width * m_height);

    switch (m_eType)
    {
        case Image::TYPE::RGB:
        {
            for (ssize i = 0; i < m_width * m_height; ++i)
            {
                const auto& pix = m_uData.pRGB[i];

                ImagePixelRGBA p;
                p.r = pix.r;
                p.g = pix.g;
                p.b = pix.b;
                p.a = 255;

                nImg.m_uData.pRGBA[i] = p;
            }
        }
        break;

        case Image::TYPE::RGBA:
        {
            utils::memCopy(nImg.m_uData.pRGBA, m_uData.pRGBA, m_width * m_height);
        }
        break;

        case Image::TYPE::MONO:
        {
            for (ssize i = 0; i < m_width * m_height; ++i)
            {
                const auto& pix = m_uData.pMono[i];

                ImagePixelRGBA p;
                p.r = pix;
                p.g = pix;
                p.b = pix;
                p.a = 255;

                nImg.m_uData.pRGBA[i] = p;
            }
        };
    }

    return nImg;
}

void
Image::swapRedBlue()
{
    using namespace adt::simd;

    switch (m_eType)
    {
        case Image::TYPE::RGBA:
        {
            const ssize size = m_width * m_height;
            ssize i = 0;

#ifdef ADT_AVX2
            const ssize divLen = size / 8;
            auto* pData = reinterpret_cast<__m256i*>(m_uData.pRGBA);
            for (; i < divLen; ++i, ++pData)
            {
                i32x8 x = i32x8Load(pData);

                i32x8 red =   (x & 0x000000ff) << 8 * 2;
                i32x8 green = (x & 0x0000ff00);
                i32x8 blue =  (x & 0x00ff0000) >> 8 * 2;
                i32x8 alpha = (x & 0xff000000);

                i32x8Store(pData, red | green | blue | alpha);
            }
            i = divLen * 8;
#else
            const ssize divLen = size / 4;
            auto* pData = reinterpret_cast<__m128i*>(m_uData.pRGBA);
            for (; i < divLen; ++i, ++pData)
            {
                i32x4 x = i32x4Load(pData);

                i32x4 red =   (x & 0x000000ff) << 8 * 2;
                i32x4 green = (x & 0x0000ff00);
                i32x4 blue =  (x & 0x00ff0000) >> 8 * 2;
                i32x4 alpha = (x & 0xff000000);

                i32x4Store(pData, red | green | blue | alpha);
            }
            i = divLen * 4;
#endif

            for (; i < size; ++i)
                utils::swap(&m_uData.pRGBA[i].r, &m_uData.pRGBA[i].b);
        }
        break;

        case Image::TYPE::RGB:
        {
            const ssize size = m_width * m_height;
            for (ssize i = 0; i < size; ++i)
                utils::swap(&m_uData.pRGB[i].r, &m_uData.pRGB[i].b);
        };
        break;

        case Image::TYPE::MONO:
        break;
    }
}

void
Image::flipVertically(adt::IAllocator* pAlloc)
{
    switch (m_eType)
    {
        case Image::TYPE::RGBA:
        {
            auto* pTemp = pAlloc->mallocV<ImagePixelRGBA>(m_width * m_height);
            defer( pAlloc->free(pTemp) );

            auto sp = spanRGBA();
            const int halfHeight = m_height / 2;

            for (int y = 0; y < halfHeight; ++y)
            {
                utils::memCopy(pTemp, &sp(0, m_height - y - 1), m_width);
                utils::memCopy(&sp(0, m_height - y - 1), &sp(0, y), m_width);
                utils::memCopy(&sp(0, y), pTemp, m_width);
            }
        }
        break;

        case Image::TYPE::RGB:
        {
            auto* pTemp = pAlloc->mallocV<ImagePixelRGB>(m_width * m_height);
            defer( pAlloc->free(pTemp) );

            auto sp = spanRGB();
            const int halfHeight = m_height / 2;

            for (int y = 0; y < halfHeight; ++y)
            {
                utils::memCopy(pTemp, &sp(0, m_height - y - 1), m_width);
                utils::memCopy(&sp(0, m_height - y - 1), &sp(0, y), m_width);
                utils::memCopy(&sp(0, y), pTemp, m_width);
            }
        }
        break;

        case Image::TYPE::MONO:
        {
            auto* pTemp = pAlloc->mallocV<adt::u8>(m_width * m_height);
            defer( pAlloc->free(pTemp) );

            auto sp = spanMono();
            const int halfHeight = m_height / 2;

            for (int y = 0; y < halfHeight; ++y)
            {
                utils::memCopy(pTemp, &sp(0, m_height - y - 1), m_width);
                utils::memCopy(&sp(0, m_height - y - 1), &sp(0, y), m_width);
                utils::memCopy(&sp(0, y), pTemp, m_width);
            }
        }
        break;
    }
}
