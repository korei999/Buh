#include "frame.hh"

#include "app.hh"
#include "config.hh"

#include "adt/file.hh"
#include "adt/simd.hh"
#include "adt/StdAllocator.hh"
#include "adt/math.hh"
#include "adt/Arena.hh"

#include <poll.h>

using namespace adt;

namespace frame
{

bool g_bRedraw = true;

static void
fillBg(
    Span2D<u32> sp,
    const int x,
    const int y,
    const int width,
    const int height,
    u32 color
)
{
    if (x < 0 || y < 0 || width < 0 || height < 0) return;

    const int maxWidth = utils::min(width, sp.width() - x);
    if (maxWidth <= 0) return;

    for (int yOff = y; yOff < height + y; ++yOff)
    {
        simd::i32Fillx4(Span<i32> {
                reinterpret_cast<i32*>(&sp(x, yOff)),
                maxWidth
            },
            color
        );
    }
}

void
run()
{
    wl_display* pDisplay = app::g_wlClient.m_pDisplay;
    pollfd pfd {.fd = wl_display_get_fd(pDisplay), .events = POLLIN, .revents {}};

    ttf::Rasterizer& rast = app::g_rasterizer;
    const int yScale = static_cast<int>(rast.m_scale);
    const int xScale = yScale * ttf::Rasterizer::X_STEP;

    f64 updateRateMS = 1000.0 * 60.0;
    for (const auto& entry : config::inl_aStatusEntries)
    {
        if (entry.updateRateMS > 0.0)
            updateRateMS = utils::min(updateRateMS, entry.updateRateMS);
    }

    Arena buffer {SIZE_1K*3};

    while (app::g_bRunning)
    {
        defer( buffer.reset() );

        wl_display_flush(pDisplay);

        const int pollStatus = poll(&pfd, 1, updateRateMS);

        if (pfd.revents & POLLIN)
            if (wl_display_dispatch(pDisplay) == -1)
                return;

        if (pollStatus == 0) g_bRedraw = true;

        if (g_bRedraw)
        {
            defer( g_bRedraw = false );

            const f64 currTime = utils::timeNowMS();
#ifndef NDEBUG
            defer( COUT("drew in: {} ms\n", utils::timeNowMS() - currTime) );
#endif

            for (wayland::Client::Bar* pBar : app::g_wlClient.m_vpBars)
            {
                wayland::Client::Bar& rBar = *pBar;

                u32* pPoolBuffer = reinterpret_cast<u32*>(rBar.m_pPoolData);
                Span2D<u32> spBuffer {pPoolBuffer, rBar.m_width, rBar.m_height, rBar.m_width};
    
                {
                    int xOff = 0;
                    const int yOff = 0;
    
                    auto clDrawString = [&](
                            const int xOffset,
                            const StringView sv,
                            const u32 fgColor,
                            const u32 bgColor,
                            const int maxAbsX = 9999999
                        ) -> int
                    {
                        int thisXOff = 0;

                        auto fg = reinterpret_cast<const ImagePixelARGBle&>(fgColor);
                        auto bg = reinterpret_cast<const ImagePixelARGBle&>(bgColor);

                        for (const wchar_t ch : StringGlyphIt(sv))
                        {
                            defer( thisXOff += xScale );
    
                            if (ch == L' ') continue;

                            MapResult<u32, ttf::Rasterizer::UV> mFoundUV {};

                            try
                            {
                                mFoundUV = rast.addOrSearchGlyph(
                                    &app::g_threadPool.scratch(),
                                    StdAllocator::inst(),
                                    &app::g_font, ch
                                );
                            }
                            catch (const AllocException& ex)
                            {
                                ex.printErrorMsg(stderr);
                                continue;
                            }
    
                            if (!mFoundUV) continue;
                            const auto [u, v] = mFoundUV.value();
    
                            const Span2D<ImagePixelARGBle> spAtlas = rast.atlasSpan();
                            const int maxx = utils::min(utils::min(rBar.m_width, maxAbsX), xOffset + thisXOff + xScale);
                            for (int y = 0; y < yScale; ++y)
                            {
                                for (int x = xOffset + thisXOff; x < maxx; ++x)
                                {
                                    if (x < 0) break;

                                    const u8 val = spAtlas((x - xOffset - thisXOff) + u, y + v).b;
                                    if (val == 0) continue;

                                    auto& rDest = reinterpret_cast<ImagePixelARGBle&>(spBuffer(
                                        x, rBar.m_height - 1 - y - yOff
                                    ));

                                    const f32 t = val / 255.0f;

                                    rDest.a = 0xff;
                                    rDest.r = (u8)(math::lerp(bg.r, fg.r, t));
                                    rDest.g = (u8)(math::lerp(bg.g, fg.g, t));
                                    rDest.b = (u8)(math::lerp(bg.b, fg.b, t));
                                }
                            }
                        }

                        return sv.size() * xScale;
                    };

                    int xOffStatus = rBar.m_width;
                    try
                    {
                        auto clDrawEntry = [&](const StringView sv, const int offset)
                        {
                            clDrawString(offset, sv, config::inl_colorScheme.status.fg, config::inl_colorScheme.status.bg);
                        };

                        VecManaged<Pair<StringView, int>> vEntryStrings {&buffer};

                        const ssize last = utils::size(config::inl_aStatusEntries) - 1;
                        for (ssize i = last; i >= 0; --i)
                        {
                            config::StatusEntry& entry = config::inl_aStatusEntries[i];

                            switch (entry.eType)
                            {
                                case config::StatusEntry::TYPE::TEXT:
                                {
                                    xOffStatus -= strlen(entry.nts) * xScale;
                                    vEntryStrings.emplace(entry.nts, xOffStatus);
                                }
                                break;

                                case config::StatusEntry::TYPE::DATE_TIME:
                                {
                                    auto clWrite = [&]
                                    {
                                        config::String64 sf {};

                                        const time_t now = time(NULL);
                                        tm tm {};
                                        localtime_r(&now, &tm);

                                        strftime(sf.data(), sf.size() - 1, entry.nts, &tm);

                                        return sf;
                                    };

                                    if (entry.lastUpdateTimeMS + entry.updateRateMS <= currTime)
                                    {
                                        entry.sfHolder = clWrite();
                                        entry.lastUpdateTimeMS = currTime;
                                    }

                                    xOffStatus -= entry.sfHolder.size() * xScale;
                                    vEntryStrings.emplace(entry.sfHolder, xOffStatus);
                                }
                                break;

                                case config::StatusEntry::TYPE::KEYBOARD_LAYOUT:
                                {
                                    xOffStatus -= rBar.m_sfKbLayout.size() * xScale;
                                    vEntryStrings.emplace(rBar.m_sfKbLayout, xOffStatus);
                                }
                                break;

                                case config::StatusEntry::TYPE::FILE_WATCH:
                                {
                                    auto clWrite = [&]
                                    {
                                        config::String64 sf = file::load<config::String64::CAP>(entry.nts);
                                        if (entry.pfnFormatString)
                                            sf = entry.pfnFormatString(sf.data());

                                        return sf;
                                    };

                                    if (entry.lastUpdateTimeMS + entry.updateRateMS <= currTime)
                                    {
                                        entry.sfHolder = clWrite();
                                        entry.lastUpdateTimeMS = currTime;
                                    }

                                    xOffStatus -= entry.sfHolder.size() * xScale;
                                    vEntryStrings.emplace(entry.sfHolder, xOffStatus);
                                }
                                break;
                            }

                            xOffStatus -= xScale*2;
                        }

                        fillBg(spBuffer, xOffStatus, 0, rBar.m_width - xOffStatus, yScale, config::inl_colorScheme.status.bg);
                        for (auto [sv, offset] : vEntryStrings) clDrawEntry(sv, offset);
                    }
                    catch (const AllocException& ex)
                    {
                        ex.printErrorMsg(stderr);
                    }
    
                    for (const Tag& tag : rBar.m_vTags)
                    {
                        const ssize tagI = rBar.m_vTags.idx(&tag);
                        char aTagStringBuff[4] {};
                        const ssize n = print::toSpan(aTagStringBuff, "{}", 1 + tagI);
                        const int tagXBegin = xOff;
                        u32 fgColor = config::inl_colorScheme.tag.fg;
                        u32 bgColor = config::inl_colorScheme.tag.bg;

                        if (tag.eState == ZDWL_IPC_OUTPUT_V2_TAG_STATE_URGENT)
                        {
                            fgColor = config::inl_colorScheme.urgentTag.fg;
                            bgColor = config::inl_colorScheme.urgentTag.bg;
                        }
                        else if (tag.eState == ZDWL_IPC_OUTPUT_V2_TAG_STATE_ACTIVE)
                        {
                            fgColor = config::inl_colorScheme.activeTag.fg;
                            bgColor = config::inl_colorScheme.activeTag.bg;
                        }

                        xOff += xScale / 4;

                        fillBg(spBuffer, tagXBegin, 0, (7*xScale)/4 + n*xScale, yScale, bgColor);

                        if (tag.nClients > 0)
                        {
                            /* lil square */
                            const int height = rBar.m_height / 5;
                            const int yOff2 = height / 1.5;
                            // for (int y = 0; y < height; ++y)
                            // {
                            //     for (int x = 0; x < height; ++x)
                            //         spBuffer(x + xOff, y + yOff2) = fgColor;
                            // }
                            fillBg(spBuffer, xOff, yOff2, height, height, fgColor);
                        }

                        xOff += xScale / 2;
                        xOff += clDrawString(xOff, StringView {aTagStringBuff, n}, fgColor, bgColor, xOffStatus);
                        xOff += xScale;
    
                        const int tagXEnd = xOff;
    
                        const int px = app::g_wlClient.m_pointer.surfacePointerX;
                        if (px >= tagXBegin && px < tagXEnd &&
                            app::g_wlClient.m_pointer.eButton == wayland::Client::Pointer::BUTTON::LEFT
                        )
                        {
                            if (app::g_wlClient.m_pointer.pLastEnterSufrace == rBar.m_pSurface)
                                zdwl_ipc_output_v2_set_tags(rBar.m_pDwlOutput, 1 << tagI, 0);
                        }
                    }

                    fillBg(spBuffer, xOff, 0, rBar.m_sfLayoutIcon.size()*xScale + xScale*2, yScale, config::inl_colorScheme.tag.bg);

                    xOff += xScale;
                    xOff += clDrawString(xOff, rBar.m_sfLayoutIcon, config::inl_colorScheme.status.fg, config::inl_colorScheme.tag.bg, xOffStatus);
                    xOff += xScale;

                    fillBg(spBuffer, xOff, 0, (xOffStatus - xOff) + xScale, yScale, config::inl_colorScheme.title.bg);

                    xOff += xScale;
                    xOff += clDrawString(xOff, rBar.m_sfTitle, config::inl_colorScheme.title.fg, config::inl_colorScheme.title.bg, xOffStatus);
                }
    
                wl_surface_attach(rBar.m_pSurface, rBar.m_pBuffer, 0, 0);
                wl_surface_damage_buffer(rBar.m_pSurface, 0, 0, rBar.m_width, rBar.m_height);
                wl_surface_commit(rBar.m_pSurface);
            }
        }

        app::g_wlClient.m_pointer.eButton = {};
        app::g_wlClient.m_pointer.state = {};
    }
}

} /* namespace frame */
