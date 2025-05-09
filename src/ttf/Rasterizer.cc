#include "Rasterizer.hh"
#include "Parser.hh"

#include "app.hh"

#include "adt/math.hh"
#include "adt/BufferAllocator.hh"
#include "adt/Array.hh"
#include "adt/logs.hh"
#include "adt/simd.hh"

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
    Vec<PointOnCurve> aNew(pAlloc, aNonCurvyPoints.size());
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
Rasterizer::rasterizeGlyph(const Parser& font, const Glyph& glyph, int xOff, int yOff)
{
    BufferAllocator allo(app::gtl_scratch.allMem<PointOnCurve>());

    CurveEndIdx endIdxs {};
    Vec<PointOnCurve> vCurvyPoints = makeItCurvy(
        &allo, pointsWithMissingOnCurve(&allo, glyph), &endIdxs, 6
    );

    const f32 xMax = font.m_head.xMax;
    const f32 xMin = font.m_head.xMin;
    const f32 yMax = font.m_head.yMax;
    const f32 yMin = font.m_head.yMin;

    Array<f32, 64> aIntersections {};
    Span2D<u8> spAtlas = m_altas.spanMono();

    const f32 hScale = static_cast<f32>(m_scale) / static_cast<f32>(xMax - xMin);
    const f32 vScale = static_cast<f32>(m_scale) / static_cast<f32>(yMax - yMin);

    for (ssize row = 0; row < static_cast<ssize>(m_scale); ++row)
    {
        aIntersections.setSize(0);
        const f32 scanline = static_cast<f32>(row);

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

            const f32 dy = y1 - y0;
            const f32 dx = x1 - x0;

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
                const int endIdx = end;
                const f32 endCovered = end - endIdx;

                for (int col = startI + 1; col < endIdx; ++col)
                    spAtlas(xOff + col, yOff + row) = 255;

                if (startI == endIdx)
                {
                    spAtlas(xOff + startI, yOff + row) += 255.0f * startCovered;
                }
                else
                {
                    spAtlas(xOff + startI, yOff + row) += 255.0f * startCovered;
                    spAtlas(xOff + endIdx, yOff + row) += 255.0f * endCovered;
                }
            }
        }
    }
}

void
Rasterizer::rasterizeAsciiIntoAltas(IAllocator* pAlloc, Parser* pFont, f32 scale)
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

    ssize nSquares = 10;
    ssize size = math::sq(scale) * math::sq(nSquares);

    m_altas.m_eType = Image::TYPE::MONO;
    m_altas.m_uData.pMono = pAlloc->zallocV<u8>(size);
    m_altas.m_width = nSquares * scale;
    m_altas.m_height = nSquares * scale;

    i16 xOff = 0;
    i16 yOff = 0;
    const i16 xStep = iScale * X_STEP;

    Span<u8> spMem = app::gtl_scratch.allMem<u8>();
    BufferAllocator arena {spMem};

    try
    {
        for (u32 ch = '!'; ch <= '~'; ++ch)
        {
            m_mapCodeToUV.insert(pAlloc, ch, {xOff, yOff});

            Glyph* pGlyph = pFont->readGlyph(ch);
            if (!pGlyph) continue;

            auto clRasterize = [this, pFont, pGlyph, xOff, yOff]
            {
                rasterizeGlyph(*pFont, *pGlyph, xOff, yOff);
            };

            auto* pCl = arena.alloc<decltype(clRasterize)>(clRasterize);

            /* no data dependency between altas regions */
            app::g_threadPool.addRetry(+[](void* pArg) -> THREAD_STATUS
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

            if ((xOff += xStep) >= (nSquares*iScale) - xStep)
            {
                xOff = 0;
                if ((yOff += iScale) >= (nSquares*iScale) - iScale)
                    break;
            }
        }
    }
    catch (const AllocException& ex)
    {
        ex.printErrorMsg(stderr);
    }

    app::g_threadPool.wait();
}

void
Rasterizer::destroy(adt::IAllocator* pAlloc)
{
    pAlloc->free(m_altas.m_uData.pMono);
    m_mapCodeToUV.destroy(pAlloc);
    *this = {};
}

} /* namespace ttf */
