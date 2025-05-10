#include "frame.hh"

#include "app.hh"
#include "config.hh"

#include "adt/logs.hh"
#include "adt/file.hh"
#include "adt/simd.hh"
#include "adt/StdAllocator.hh"
#include "adt/math.hh"

#include <poll.h>

using namespace adt;

namespace frame
{

bool g_bRedraw = false;

void
run()
{
    wl_display* pDisplay = app::g_wlClient.m_pDisplay;
    pollfd pfd {.fd = wl_display_get_fd(pDisplay), .events = POLLIN, .revents {}};

    ttf::Rasterizer& rast = app::g_rasterizer;
    const int scale = static_cast<int>(rast.m_scale);
    const int xScale = scale * ttf::Rasterizer::X_STEP;

    f64 updateRateMS = 1000.0 * 60.0;
    for (const auto& entry : config::inl_aStatusEntries)
    {
        if (entry.updateRateMS > 0.0)
            updateRateMS = utils::min(updateRateMS, entry.updateRateMS);
    }

    while (app::g_bRunning)
    {
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
                wayland::Client::Bar& rbar = *pBar;

                u32* p = reinterpret_cast<u32*>(rbar.m_pPoolData);
                Span2D<u32> spBuffer {p, rbar.m_width, rbar.m_height, rbar.m_width};
    
#ifdef ADT_AVX2
                simd::i32Fillx8({reinterpret_cast<i32*>(p), rbar.m_width * rbar.m_height}, 0xff777777);
#else
                simd::i32Fillx4({reinterpret_cast<i32*>(p), rbar.m_width * rbar.m_height}, 0xff777777);
#endif
    
                {
                    int xOff = 0;
                    const int yOff = 0;
    
                    auto clDrawString = [&](
                            const int xOffset,
                            const StringView sv,
                            const u32 color,
                            const int maxAbsX = 9999999
                        ) -> int
                    {
                        int thisXOff = 0;

                        const u8 penR = (color >> 16) & 0xff;
                        const u8 penG = (color >> 8) & 0xff;
                        const u8 penB = (color >> 0) & 0xff;

                        for (const wchar_t ch : StringGlyphIt(sv))
                        {
                            defer( thisXOff += xScale );
    
                            if (ch == L' ') continue;

                            try
                            {
                                const MapResult<u32, Pair<i16, i16>> mRes = rast.addOrSearchGlyph(
                                    &app::g_threadPool.scratch(),
                                    StdAllocator::inst(),
                                    &app::g_font, ch
                                );
                                if (!mRes) continue;
    
                                const auto [u, v] = mRes.value();
    
                                const Span2D<u8> spAtlas = rast.atlasSpan();
                                const int maxx = utils::min(utils::min(rbar.m_width, maxAbsX), xOffset + thisXOff + xScale);
                                for (int y = 0; y < scale; ++y)
                                {
                                    for (int x = xOffset + thisXOff; x < maxx; ++x)
                                    {
                                        const u8 val = spAtlas((x - xOffset - thisXOff) + u, y + v);
                                        if (val == 0) continue;

                                        auto& rDest = reinterpret_cast<ImagePixelARGBle&>(spBuffer(
                                            x, rbar.m_height - 1 - y - yOff
                                        ));

                                        const f32 t = val / 255.0f;

                                        rDest.a = 0xff;
                                        rDest.r = (u8)(math::lerp(rDest.r, penR, t));
                                        rDest.g = (u8)(math::lerp(rDest.g, penG, t));
                                        rDest.b = (u8)(math::lerp(rDest.b, penB, t));
                                    }
                                }
                            }
                            catch (const AllocException& ex)
                            {
                                ex.printErrorMsg(stderr);
                            }
                        }

                        return sv.size() * xScale;
                    };
    
                    int xOffStatus = rbar.m_width;
                    {
                        auto clDrawEntry = [&](const StringView sv)
                        {
                            xOffStatus -= sv.size() * xScale;
                            clDrawString(xOffStatus, sv, 0xff000000);
                        };

                        const ssize last = utils::size(config::inl_aStatusEntries) - 1;
                        for (ssize i = last; i >= 0; --i)
                        {
                            config::StatusEntry& entry = config::inl_aStatusEntries[i];
                            switch (entry.eType)
                            {
                                case config::StatusEntry::TYPE::TEXT:
                                clDrawEntry(entry.nts);
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

                                    clDrawEntry(entry.sfHolder);
                                }
                                break;

                                case config::StatusEntry::TYPE::KEYBOARD_LAYOUT:
                                clDrawEntry(rbar.m_sfKbLayout);
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

                                    clDrawEntry(entry.sfHolder);
                                }
                                break;
                            }
                            xOffStatus -= xScale*2;
                        }
                    }
    
                    for (const Tag& tag : rbar.m_vTags)
                    {
                        const ssize tagI = rbar.m_vTags.idx(&tag);
                        char aBuff[4] {};
                        const ssize n = print::toSpan(aBuff, "{}", 1 + tagI);
                        const int tagXBegin = xOff;

                        if (tag.eState == ZDWL_IPC_OUTPUT_V2_TAG_STATE_URGENT)
                        {
                            const int numberLen = (n*xScale) + xScale/2 + xScale;
                            for (int y = 0; y < scale; ++y)
                            {
#ifdef ADT_AVX2
                                simd::i32Fillx8({(i32*)&spBuffer(xOff, y), numberLen}, 0xffac4242);
#else
                                simd::i32Fillx4({(i32*)&spBuffer(xOff, y), numberLen}, 0xffac4242);
#endif
                            }
                        }

                        const u32 color = [&]
                        {
                            if (tag.eState == ZDWL_IPC_OUTPUT_V2_TAG_STATE_ACTIVE ||
                                tag.eState == ZDWL_IPC_OUTPUT_V2_TAG_STATE_URGENT
                            )
                            {
                                return 0xff000000U;
                            }
                            else
                            {
                                return 0xff555555U;
                            }
                        }();
    
                        xOff += xScale / 4;

                        if (tag.nClients > 0)
                        {
                            /* draw lil something */
                            const int height = rbar.m_height / 5;
                            const int yOff2 = height / 1.5;
                            for (int y = 0; y < height; ++y)
                            {
                                for (int x = 0; x < height; ++x)
                                    spBuffer(x + xOff, y + yOff2) = color;
                            }
                        }
    
                        xOff += xScale / 2;
                        xOff += clDrawString(xOff, StringView {aBuff, n}, color, xOffStatus);
                        xOff += xScale;
    
                        const int tagXEnd = xOff;
    
                        const int px = app::g_wlClient.m_pointer.surfacePointerX;
                        if (px >= tagXBegin && px < tagXEnd &&
                            app::g_wlClient.m_pointer.eButton == wayland::Client::Pointer::BUTTON::LEFT
                        )
                        {
                            if (app::g_wlClient.m_pointer.pLastEnterSufrace == rbar.m_pSurface)
                                zdwl_ipc_output_v2_set_tags(rbar.m_pDwlOutput, 1 << tagI, 0);
                        }
                    }
                    xOff += xScale;
    
                    xOff += clDrawString(xOff, rbar.m_sfLayoutIcon, 0xff000000, xOffStatus);
                    xOff += xScale * 2;
                    xOff += clDrawString(xOff, rbar.m_sfTitle, 0xff000000, xOffStatus);
                }
    
                wl_surface_attach(rbar.m_pSurface, rbar.m_pBuffer, 0, 0);
                wl_surface_damage_buffer(rbar.m_pSurface, 0, 0, rbar.m_width, rbar.m_height);
                wl_surface_commit(rbar.m_pSurface);
            }
        }

        app::g_wlClient.m_pointer.eButton = {};
        app::g_wlClient.m_pointer.state = {};
    }
}

} /* namespace frame */
