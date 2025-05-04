#include "frame.hh"

#include "app.hh"

#include "adt/logs.hh"
#include "adt/simd.hh"

#include <poll.h>

using namespace adt;

namespace frame
{

bool g_bRedraw = true;

void
run()
{
    wl_display* pDisplay = app::client().m_pDisplay;
    // const int fdDisplay = wl_display_get_fd(pDisplay);
    // pollfd pfd {.fd = fdDisplay, .events = POLLIN, .revents {}};

    const ttf::Rasterizer& rast = app::g_rasterizer;
    const int scale = static_cast<int>(rast.m_scale);
    const int xScale = scale * ttf::Rasterizer::X_STEP;

    while (app::g_bRunning)
    {
        ADT_ASSERT_ALWAYS(
            wl_display_dispatch(pDisplay) >= 0,
            ""
        );

        if (g_bRedraw)
        {
            for (auto& bar : app::client().m_vOutputBars)
            {
                u32* p = reinterpret_cast<u32*>(app::client().m_pPoolData);
                Span2D<u32> spBuffer {p, bar.m_width, bar.m_height, bar.m_width};

                simd::i32Fillx4({(i32*)p, bar.m_width * bar.m_height}, 0xff777777);

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
                        for (const char ch : sv)
                        {
                            defer( thisXOff += xScale );

                            if (ch == ' ') continue;

                            MapResult mRes = rast.m_mapCodeToUV.search(ch);
                            if (mRes.eStatus == MAP_RESULT_STATUS::NOT_FOUND) continue;

                            Pair<i16, i16> uv = mRes.value();

                            Span2D<u8> spMono = rast.m_altas.spanMono();

                            for (int y = 0; y < scale; ++y)
                            {
                                for (int x = 0; x < xScale; ++x)
                                {
                                    const int off = x + xOffset + thisXOff;
                                    if (off >= bar.m_width || off >= maxAbsX) goto GOTO_done;

                                    const u8 val = spMono(x + uv.first, y + uv.second);
                                    if (val > 0) spBuffer(x + xOffset + thisXOff, bar.m_height - y - yOff) = color;
                                }
                            }
                        }
GOTO_done:
                        return thisXOff;
                    };

                    int xOffStatus = bar.m_width - (bar.m_sfKbLayout.size() * xScale);
                    clDrawString(xOffStatus, bar.m_sfKbLayout, 0xff000000);

                    {
                        time_t now = time(NULL);
                        struct tm tm {};
                        localtime_r(&now, &tm);

                        char aBuff[64] {};
                        const int n = strftime(aBuff, sizeof(aBuff) - 1, "%Y-%m-%d %I:%M%p", &tm);
                        const int off = (n + 2)*xScale;
                        clDrawString(xOffStatus - off, StringView {aBuff, n}, 0xff000000);
                        xOffStatus -= off;
                    }

                    xOffStatus -= xScale;

                    for (const Tag& tag : bar.m_vTags)
                    {
                        const ssize tagI = bar.m_vTags.idx(&tag);
                        char aBuff[4] {};
                        const ssize n = print::toSpan(aBuff, "{}", 1 + tagI);

                        const u32 color = [&]
                        {
                            if (tag.eState == ZDWL_IPC_OUTPUT_V2_TAG_STATE_ACTIVE)
                                return 0xff000000U;
                            else return 0xff555555U;
                        }();

                        xOff += xScale / 4;
                        if (tag.nClients > 0)
                        {
                            /* draw lil something */
                            const int height = bar.m_height / 5;
                            const int yOff2 = height / 1.5;
                            for (int y = 0; y < height; ++y)
                            {
                                for (int x = 0; x < height; ++x)
                                    spBuffer(x + xOff, y + yOff2) = color;
                            }
                        }

                        const int tagXBegin = xOff;

                        xOff += xScale / 2;
                        xOff += clDrawString(xOff, StringView {aBuff, n}, color, xOffStatus);
                        xOff += xScale;

                        const int tagXEnd = xOff;

                        const int px = app::client().m_pointer.surfacePointerX;
                        const int py = app::client().m_pointer.surfacePointerY;
                        if (px >= tagXBegin && px < tagXEnd &&
                            app::client().m_pointer.eButton == wl::Client::Pointer::BUTTON::LEFT
                        )
                        {
                            zdwl_ipc_output_v2_set_tags(bar.m_pDwlOutput, 1 << tagI, 0);
                        }
                    }
                    xOff += xScale;

                    xOff += clDrawString(xOff, bar.m_sfLayoutIcon, 0xff000000, xOffStatus);
                    xOff += xScale * 2;
                    xOff += clDrawString(xOff, bar.m_sfTitle, 0xff000000, xOffStatus);
                }

                wl_surface_attach(bar.m_pSurface, bar.m_pBuffer, 0, 0);
                wl_surface_damage_buffer(bar.m_pSurface, 0, 0, bar.m_width, bar.m_height);
                wl_surface_commit(bar.m_pSurface);
            }
            g_bRedraw = false;
        }

        app::client().m_pointer.eButton = {};
        app::client().m_pointer.state = {};
    }
}

} /* namespace frame */
