/* https://en.wikipedia.org/wiki/BMP_file_format */

#pragma once

#include "Image.hh"

#include "adt/String.hh" /* IWYU pragma: keep */
#include "adt/defer.hh"
#include "adt/logs.hh"

namespace BMP
{

enum class COMPRESSION_METHOD_ID : adt::u32
{
    RGB = 0,
    RLE8 = 1,
    RLE4 = 2,
    BITFIELDS = 3,
    JPEG = 4,
    PNG = 5,
    ALPHABITFIELDS = 6,
    CMYK = 11,
    CMYKRLE8 = 12,
    CMYKRLE4 = 13,
};

#pragma pack(1)
struct BitmapInfoHeader
{
    adt::u32 size {}; /* the size of this header, in bytes (40) */
    adt::i32 width {}; /* the bitmap width in pixels (signed integer) */
    adt::i32 height {}; /* the bitmap height in pixels (signed integer) */
    adt::u16 nPlanes {}; /* the number of color planes (must be 1) */
    adt::u16 nBitsPerPixel {}; /* the number of bits per pixel, which is the color depth of the image. Typical values are 1, 4, 8, 16, 24 and 32. */
    COMPRESSION_METHOD_ID eCompressionMethod {};
    adt::u32 imageSize {}; /* the image size. This is the size of the raw bitmap data; a dummy 0 can be given for BI_RGB bitmaps. */
    adt::i32 hRes {}; /* the horizontal resolution of the image. (pixel per metre, signed integer) */
    adt::i32 vRes {}; /* the vertical resolution of the image. (pixel per metre, signed integer) */
    adt::u32 nColors {}; /* the number of colors in the color palette, or 0 to default to 2^n */
    adt::u32 nColorsUsed {}; /* the number of important colors used, or 0 when every color is important; generally ignored */
};
#pragma pack()

#pragma pack(1)
struct Header
{
    /* 'BM' header field used to identify the BMP and DIB file is 0x42 0x4D in hexadecimal, same as BM in ASCII. The following entries are possible:
     * BM Windows 3.1x, 95, NT, ... etc.
     * BA OS/2 struct bitmap array
     * CI OS/2 struct color icon
     * CP OS/2 const color pointer
     * IC OS/2 struct icon
     * PT OS/2 pointer */
    adt::u16 BM {};
    adt::u32 size {}; /* The size of the BMP file in bytes */
    adt::u16 _reserved0 {}; /* actual value depends on the application that creates the image, if created manually can be 0 */
    adt::u16 _reserved1 {}; /* actual value depends on the application that creates the image, if created manually can be 0 */
    adt::u32 offset {}; /* i.e. starting address, of the byte where the bitmap image data (pixel array) can be found. */
};
#pragma pack()

struct Reader
{
    adt::StringView m_sBMP {};
    Header m_header {};
    BitmapInfoHeader m_bmInfoHeader {};

    /* */

    Reader() = default;

    /* */

    [[nodiscard]] bool read(adt::StringView sBMP);
    Image getImage();

private:
    bool parse();
};

inline bool
Reader::read(adt::StringView sBMP)
{
    using namespace adt;

    if (sBMP.size() < static_cast<ssize>(sizeof(Header) + sizeof(BitmapInfoHeader)))
        return false;

    m_sBMP = sBMP;

    return parse();
}

inline Image
Reader::getImage()
{
    using namespace adt;

    if (m_sBMP.size() == 0)
        return {};

    switch (m_bmInfoHeader.nBitsPerPixel)
    {
        case 24:
        {
            return {
                .m_uData {.pRGB = reinterpret_cast<ImagePixelRGB*>(&m_sBMP[m_header.offset])},
                .m_width = static_cast<i16>(m_bmInfoHeader.width),
                .m_height = static_cast<i16>(m_bmInfoHeader.height),
                .m_eType = Image::TYPE::RGB,
            };
        }
        break;

        case 32:
        {
            return {
                .m_uData {.pRGBA = reinterpret_cast<ImagePixelRGBA*>(&m_sBMP[m_header.offset])},
                .m_width = static_cast<i16>(m_bmInfoHeader.width),
                .m_height = static_cast<i16>(m_bmInfoHeader.height),
                .m_eType = Image::TYPE::RGBA,
            };
        }
        break;
    }

    return {};
}

inline bool
Reader::parse()
{
    using namespace adt;

    m_header = m_sBMP.reinterpret<Header>(0);
#ifdef DBG_BMP
    LOG_NOTIFY("Header\n"
        "BM: '{}'\n"
        "size: {}\n"
        "offset: {}\n",
        String((char*)&m_header.BM, 2),
        m_header.size, m_header.offset
    );
#endif /* DBG_BMP */

    m_bmInfoHeader = m_sBMP.reinterpret<BitmapInfoHeader>(sizeof(Header));
#ifdef DBG_BMP
    LOG_NOTIFY("BitmapInfoHeader\n"
        "size: {}\n"
        "width: {}\n"
        "height: {}\n"
        "nPlanes: {}\n"
        "nBitsPerPixel: {}\n"
        "eCompressionMethod: '{}'\n"
        "imageSize: {}\n"
        "hRes: {}\n"
        "vRes: {}\n"
        "nColors: {}\n"
        "nColorsUsed: {}\n",
        m_bmInfoHeader.size, m_bmInfoHeader.width, m_bmInfoHeader.height, 
        m_bmInfoHeader.nPlanes, m_bmInfoHeader.nBitsPerPixel,
        m_bmInfoHeader.eCompressionMethod, m_bmInfoHeader.nPlanes,
        m_bmInfoHeader.imageSize, m_bmInfoHeader.hRes, m_bmInfoHeader.vRes,
        m_bmInfoHeader.nColors, m_bmInfoHeader.nColorsUsed
    );
#endif /* DBG_BMP */

    return true;
}

inline void
writeToFile(const Image img, const char* ntsFile)
{
    FILE* fp = fopen(ntsFile, "wb");
    if (!fp)
    {
        LOG_BAD("fpen(\"{}\", \"wb\") failed\n", ntsFile);
        return;
    }
    defer( fclose(fp) );

    adt::u16 nBitsPerPixel = [&]
    {
        switch (img.m_eType)
        {
            case Image::TYPE::MONO:
            return 8;

            default:
            ADT_ASSERT(false, "unhandled");
        }

        return 0;
    }();

    Header head {
        .BM = adt::u32('B' | 'M' << 8),
        .size = adt::u32(sizeof(Header) + sizeof(BitmapInfoHeader) + img.m_width*img.m_height),
        .offset = sizeof(Header) + sizeof(BitmapInfoHeader),
    };

    BitmapInfoHeader info {
        .size = sizeof(BitmapInfoHeader),
        .width = img.m_width,
        .height = img.m_height,
        .nPlanes = 1,
        .nBitsPerPixel = nBitsPerPixel,
        .eCompressionMethod = COMPRESSION_METHOD_ID::RGB,
        .imageSize = 1,
        .hRes = 0,
        .vRes = 0,
        .nColors = 0,
        .nColorsUsed = 0,
    };

    fwrite(&head, sizeof(head), 1, fp);
    fwrite(&info, sizeof(info), 1, fp);
    fwrite(img.spanMono().data(), img.m_width * img.m_height, 1, fp);

    // LOG_NOTIFY("written: w/h: [{}, {}]\n", img.m_width, img.m_height);
}

};

#ifdef DBG_BMP
namespace adt::print
{

inline ssize
formatToContext(Context ctx, FormatArgs fmtArgs, bmp::COMPRESSION_METHOD_ID eCompressionMethod) noexcept
{
    constexpr String asMethods[] {
        "BI_RGB",
        "BI_RLE8",
        "BI_RLE4",
        "BI_BITFIELDS",
        "BI_JPEG",
        "BI_PNG",
        "BI_ALPHABITFIELDS",
        "BI_CMYK",
        "BI_CMYKRLE8",
        "BI_CMYKRLE4",
    };

    ADT_ASSERT(int(eCompressionMethod) < utils::size(asMethods), " ");

    return formatToContext(ctx, fmtArgs, asMethods[eCompressionMethod]);
}

} /* namespace adt::print */
#endif /* DBG_BMP */
