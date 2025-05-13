#include "Rasterizer.hh"
#include "Parser.hh"

#include "adt/math.hh"
#include "adt/BufferAllocator.hh"
#include "adt/Array.hh"
#include "adt/logs.hh"

using namespace adt;

namespace ttf
{

struct CurveEndIdx
{
    static constexpr int CAP = 8;

    i16 aIdxs[CAP];
};

struct PointOnCurve
{
    math::V2 pos;
    bool bOnCurve;
    bool bEndOfCurve;
    /*f32 _pad {};*/
};

Vec<PointOnCurve>
pointsWithMissingOnCurve(IAllocator* pAlloc, const Glyph& g)
{
    const auto& aGlyphPoints = g.uGlyph.simple.vPoints;
    const u32 size = aGlyphPoints.size();

    bool bCurrOnCurve = false;
    bool bPrevOnCurve = false;

    [[maybe_unused]] ssize firstInCurveIdx = 0;

    Vec<PointOnCurve> vPoints(pAlloc, size);
    for (const auto& p : aGlyphPoints)
    {
        const u32 pointIdx = aGlyphPoints.idx(&p);

        f32 x = f32(p.x);
        f32 y = f32(p.y);

        bool bEndOfCurve = false;
        for (auto e : g.uGlyph.simple.vEndPtsOfContours)
            if (e == pointIdx)
                bEndOfCurve = true;

        math::V2 vCurr {x, y};

        bCurrOnCurve = p.bOnCurve;
        defer( bPrevOnCurve = bCurrOnCurve );

        if (!bCurrOnCurve && !bPrevOnCurve)
        {
            /* insert middle point */
            const auto& prev = vPoints.last();
            math::V2 mid = math::lerp(prev.pos, vCurr, 0.5f);

            vPoints.push(pAlloc, {
                .pos = mid,
                .bOnCurve = true,
                .bEndOfCurve = false
            });
        }

        vPoints.push(pAlloc, {
            .pos {x, y},
            .bOnCurve = p.bOnCurve,
            .bEndOfCurve = bEndOfCurve
        });

#ifndef NDEBUG
        if (!bCurrOnCurve && bEndOfCurve)
            ADT_ASSERT(aGlyphPoints[firstInCurveIdx].bOnCurve == true, " ");
#endif

        if (bEndOfCurve) firstInCurveIdx = pointIdx + 1;
    }

    return vPoints;
}

static void
insertPoints(
    IAllocator* pAlloc,
    Vec<PointOnCurve>* aPoints,
    const math::V2& p0,
    const math::V2& p1,
    const math::V2& p2,
    int nTimes = 1
)
{
    for (int i = 1; i < nTimes + 1; ++i)
    {
        f32 t = f32(i) / f32(nTimes + 1);

        auto point = math::bezier(p0, p1, p2, t);
        aPoints->push(pAlloc, {
            .pos = point,
            .bOnCurve = true,
            .bEndOfCurve = false
        });
    }
}

static Vec<PointOnCurve>
makeItCurvy(IAllocator* pAlloc, const Vec<PointOnCurve>& aNonCurvyPoints, CurveEndIdx* pEndIdxs, int nTessellations)
{
    Vec<PointOnCurve> aNew {pAlloc, aNonCurvyPoints.size()};
    simd::i16Fillx8(pEndIdxs->aIdxs, -1);
    i16 endIdx = 0;

    ssize firstInCurveIdx = 0;
    bool bPrevOnCurve = true;
    for (auto& p : aNonCurvyPoints)
    {
        ssize idx = aNonCurvyPoints.idx(&p);

        if (p.bEndOfCurve)
        {
            if (!aNonCurvyPoints[idx].bOnCurve || !aNonCurvyPoints[firstInCurveIdx].bOnCurve)
            {
                if (nTessellations > 0)
                {
                    math::V2 p0 {aNonCurvyPoints[idx - 1].pos};
                    math::V2 p1 {aNonCurvyPoints[idx - 0].pos};
                    math::V2 p2 {aNonCurvyPoints[firstInCurveIdx].pos};
                    insertPoints(pAlloc, &aNew, p0, p1, p2, nTessellations);
                }
            }
        }

        if (!bPrevOnCurve)
        {
            if (nTessellations > 0)
            {
                math::V2 p0 {aNonCurvyPoints[idx - 2].pos};
                math::V2 p1 {aNonCurvyPoints[idx - 1].pos};
                math::V2 p2 {aNonCurvyPoints[idx - 0].pos};
                insertPoints(pAlloc, &aNew, p0, p1, p2, nTessellations);
            }
        }

        if (p.bOnCurve)
        {
            aNew.push(pAlloc, {
                .pos = p.pos,
                .bOnCurve = p.bOnCurve,
                .bEndOfCurve = false
            });
        }

        if (p.bEndOfCurve)
        {
            aNew.push(pAlloc, {
                .pos = aNonCurvyPoints[firstInCurveIdx].pos,
                .bOnCurve = true,
                .bEndOfCurve = true,
            });

            if (endIdx < 8) pEndIdxs->aIdxs[endIdx++] = aNew.lastI();
            else ADT_ASSERT(false, "8 curves max");

            firstInCurveIdx = idx + 1;
            bPrevOnCurve = true;
        }
        else
        {
            bPrevOnCurve = p.bOnCurve;
        }
    }

    return aNew;
}

void
Rasterizer::rasterizeGlyph(ScratchBuffer* pScratch, const Parser& font, const Glyph& glyph, int xOff, int yOff)
{
    BufferAllocator buff {pScratch->nextMem<u8>()};

    CurveEndIdx endIdxs {};
    Vec<PointOnCurve> vCurvyPoints = makeItCurvy(
        &buff, pointsWithMissingOnCurve(&buff, glyph), &endIdxs, 6
    );

    const f32 xMax = font.m_head.xMax;
    const f32 xMin = font.m_head.xMin;
    const f32 yMax = font.m_head.yMax;
    const f32 yMin = font.m_head.yMin;

    Array<f32, 64> aIntersections {};
    Span2D<ImagePixelARGBle> spAtlas = atlasSpan();

    const f32 hScale = static_cast<f32>(m_scale) / static_cast<f32>(xMax - xMin);
    const f32 vScale = static_cast<f32>(m_scale) / static_cast<f32>(yMax - yMin);

    constexpr ssize scanlineSubdiv = 5;
    constexpr f32 alphaWeight = 255.0f / scanlineSubdiv;
    constexpr f32 stepPerScanline = 1.0f / scanlineSubdiv;

    for (ssize row = 0; row < static_cast<ssize>(m_scale); ++row)
    {
        for (ssize subRow = 0; subRow < scanlineSubdiv; ++subRow)
        {
            aIntersections.setSize(0);
            const f32 scanline = static_cast<f32>(row) + subRow*stepPerScanline;

            for (ssize pointI = 1; pointI < vCurvyPoints.size(); ++pointI)
            {
                const f32 x0 = (vCurvyPoints[pointI - 1].pos.x) * hScale;
                const f32 x1 = (vCurvyPoints[pointI - 0].pos.x) * hScale;

                const f32 y0 = (vCurvyPoints[pointI - 1].pos.y - yMin) * vScale;
                const f32 y1 = (vCurvyPoints[pointI - 0].pos.y - yMin) * vScale;

                if (vCurvyPoints[pointI].bEndOfCurve)
                    ++pointI;

                const auto [smallerY, biggerY] = utils::minMax(y0, y1);

                if (scanline <= smallerY || scanline > biggerY) continue;

                /* Scanline: horizontal line that intersects edges. Find X for scanline Y.
                 * |
                 * |      /(x1, y1)
                 * |____/______________inter(x, y)
                 * |  /
                 * |/(x0, y0)
                 * +------------ */

                const f32 dx = x1 - x0;
                const f32 dy = y1 - y0;
                const f32 interX = (scanline - y1)/dy * dx + x1;

                aIntersections.push(interX);
            }

            if (aIntersections.size() > 1)
            {
                sort::insertion(&aIntersections);

                for (ssize intI = 1; intI < aIntersections.size(); intI += 2)
                {
                    const f32 start = aIntersections[intI - 1];
                    const int startI = start;
                    const f32 startCovered = (startI + 1) - start;

                    const f32 end = aIntersections[intI];
                    const int endI = end;
                    const f32 endCovered = end - endI;

                    for (int col = startI + 1; col < endI; ++col)
                        spAtlas(xOff + col, yOff + row).b += alphaWeight;

                    if (startI == endI)
                    {
                        spAtlas.tryAt(xOff + startI, yOff + row, [&](ImagePixelARGBle& pix) { pix.b += u8(alphaWeight*startCovered); });
                    }
                    else
                    {
                        spAtlas.tryAt(xOff + startI, yOff + row, [&](ImagePixelARGBle& pix) { pix.b += u8(alphaWeight*startCovered); });
                        spAtlas.tryAt(xOff + endI, yOff + row, [&](ImagePixelARGBle& pix) { pix.b += u8(alphaWeight*endCovered); });
                    }
                }
            }
        }
    }
}

adt::MapResult<u32, Rasterizer::UV>
Rasterizer::addOrSearchGlyph(ScratchBuffer* pScratch, IAllocator* pAlloc, Parser* pFont, u32 code)
{
    auto mFound = m_mapCodeToUV.search(code);
    if (mFound) return mFound;

    Glyph* pGlyph = pFont->readGlyph(code);
    if (!pGlyph)
    {
        LOG_BAD("failed to find glyph for '{}'({})\n", wchar_t(code), code);
        return {};
    }

    const int yStep = std::round(m_scale);
    const i16 xStep = yStep * X_STEP;

    rasterizeGlyph(pScratch, *pFont, *pGlyph, m_xOffAtlas, m_yOffAtlas);
    auto mapRes = m_mapCodeToUV.insert(pAlloc, code, {m_xOffAtlas, m_yOffAtlas});

    if ((m_xOffAtlas += xStep) > (m_atlas.m_width) - xStep)
    {
        m_xOffAtlas = 0;
        if ((m_yOffAtlas += yStep) > (m_atlas.m_height) - yStep)
        {
            LOG_BAD("REALLOC: yOff: {}, height: {}\n", m_yOffAtlas, m_atlas.m_height - yStep);

            m_atlas.m_uData.pARGBle = pAlloc->reallocV<ImagePixelARGBle>(
                m_atlas.m_uData.pARGBle,
                m_atlas.m_width * m_atlas.m_height,
                m_atlas.m_width * (m_atlas.m_height*2)
            );

            m_atlas.m_height *= 2;
        }
    }

    return mapRes;
}

void
Rasterizer::destroy(adt::IAllocator* pAlloc)
{
    pAlloc->free(m_atlas.m_uData.pARGBle);
    m_mapCodeToUV.destroy(pAlloc);
    *this = {};
}

void
Rasterizer::rasterizeAscii(IAllocator* pAlloc, Parser* pFont, IThreadPoolWithMemory* pThreadPool, f32 scale)
{
    if (!pAlloc)
    {
        LOG_WARN("pAlloc: {}\n", pAlloc);
        return;
    }

    if (!pFont)
    {
        LOG_WARN("pFont: {}\n", pFont);
        return;
    }

    const int iScale = std::round(scale);
    m_scale = scale;

    constexpr ssize width = 10;
    constexpr ssize height = 10;

    const ssize size = width * scale * height * scale;;

    m_atlas.m_eType = Image::TYPE::ARGB_LE;
    m_atlas.m_uData.pARGBle = pAlloc->zallocV<ImagePixelARGBle>(size);
    m_atlas.m_width = width * scale;
    m_atlas.m_height = height * scale;

    const i16 xStep = iScale * X_STEP;

    BufferAllocator buff {pThreadPool->scratch().template nextMem<u8>()};

    try
    {
        for (u32 ch = '!'; ch <= '~'; ++ch)
        {
            Glyph* pGlyph = pFont->readGlyph(ch);
            if (!pGlyph) continue;

            m_mapCodeToUV.insert(pAlloc, ch, {m_xOffAtlas, m_yOffAtlas});

            const i16 xOff = m_xOffAtlas;
            const i16 yOff = m_yOffAtlas;
            auto clRasterize = [this, pFont, pGlyph, xOff, yOff, pThreadPool]
            {
                rasterizeGlyph(&pThreadPool->scratch(), *pFont, *pGlyph, xOff, yOff);
            };

            auto* pCl = buff.alloc<decltype(clRasterize)>(clRasterize);

            /* no data dependency between altas regions */
            pThreadPool->addRetry(+[](void* pArg) -> THREAD_STATUS
                {
                    auto& task = *static_cast<decltype(clRasterize)*>(pArg);

                    try
                    {
                        task();
                    }
                    catch (const AllocException& ex)
                    {
                        ex.printErrorMsg(stderr);
                    }

                    return THREAD_STATUS(0);
                },
                pCl
            );

            if ((m_xOffAtlas += xStep) > (m_atlas.m_width) - xStep)
            {
                m_xOffAtlas = 0;
                if ((m_yOffAtlas += iScale) > (m_atlas.m_height) - iScale)
                {
                    ADT_ASSERT(false, "unhandled");
                }
            }
        }
    }
    catch (const AllocException& ex)
    {
        ex.printErrorMsg(stderr);
    }

    pThreadPool->wait();
}

} /* namespace ttf */
