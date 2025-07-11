#include "frame.hh"

#include "app.hh"
#include "exec.hh"

#include "adt/file.hh"
#include "adt/simd.hh"
#include "adt/StdAllocator.hh"
#include "adt/math.hh"
#include "adt/Arena.hh"

#include <signal.h>
#include <poll.h>
#include <sys/signalfd.h>

#include "config.hh"

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
    const u32 color
)
{
    if (x < 0 || y < 0 || width < 0 || height < 0) return;

    const int maxWidth = utils::min(width, sp.width() - x);
    if (maxWidth <= 0) return;
    const int maxHeight = utils::min(height, sp.height() - y);

    for (int yOff = y; yOff < maxHeight + y; ++yOff)
    {
        simd::i32Fillx4(Span<i32> {
                reinterpret_cast<i32*>(&sp(x, yOff)),
                maxWidth
            },
            color
        );
    }
}

static void
fillBgOutline(
    Span2D<u32> sp,
    const int x,
    const int y,
    const int width,
    const int height,
    const int thick,
    const u32 color
)
{
    if (x < 0 || y < 0 || width < 0 || height < 0) return;

    if (thick > height) return;
    const int maxWidth = utils::min(width, sp.width() - x);
    if (maxWidth <= 0) return;
    const int maxHeight = utils::min(height, sp.height() - y);

    /* left */
    for (int yOff = y; yOff < maxHeight + y; ++yOff)
    {
        for (int xOff = x; xOff < x + thick; ++xOff)
            sp(xOff, yOff) = color;
    }

    /* right */
    for (int yOff = y; yOff < maxHeight + y; ++yOff)
    {
        for (int xOff = x + (width - thick); xOff < width + x; ++xOff)
            sp(xOff, yOff) = color;
    }

    /* top */
    for (int yOff = y; yOff < y + thick; ++yOff)
    {
        for (int xOff = x + 1; xOff < x + width - 1; ++xOff)
            sp(xOff, yOff) = color;
    }

    /* bop */
    for (int yOff = y + (height - thick); yOff < maxHeight + y; ++yOff)
    {
        for (int xOff = x + 1; xOff < x + width - 1; ++xOff)
            sp(xOff, yOff) = color;
    }
}

constexpr isize MAX_SIGNAL = 20;

void
run()
{
    sigset_t sigMask {};
    sigemptyset(&sigMask);
    sigaddset(&sigMask, SIGUSR1);
    sigaddset(&sigMask, SIGUSR2);
    for (int i = SIGRTMIN; i <= SIGRTMIN + MAX_SIGNAL; ++i)
        sigaddset(&sigMask, i);

    sigprocmask(SIG_BLOCK, &sigMask, nullptr);

    int fdSignals = signalfd(-1, &sigMask, SFD_NONBLOCK | SFD_CLOEXEC);
    pollfd sigPollFd = {.fd = fdSignals, .events = POLLIN, .revents {}};
    defer( close(fdSignals) );

    wl_display* pDisplay = app::g_wlClient.m_pDisplay;
    pollfd wlFd {.fd = wl_display_get_fd(pDisplay), .events = POLLIN, .revents {}};

    pollfd aFds[2] {wlFd, sigPollFd};

    ttf::Rasterizer& rRast = app::g_rasterizer;
    const int yScale = int(rRast.m_scale); /* y-axis rasterization scale */
    const int xScale = yScale * ttf::Rasterizer::X_STEP; /* x-axis rasterization scale */
    const int xMove = int(f32(xScale) * config::X_ADVANCE_MUL); /* distance between characters */

    const f64 updateRateMS = [&]
    {
        f64 ret = 1000.0 * 60.0;
        for (const auto& e : config::g_aStatusEntries)
            if (e.updateRateMS > 0.1)
                ret = utils::min(ret, e.updateRateMS);
        return ret;
    }();

    Arena arena {SIZE_1K*3};

    while (app::g_bRunning)
    {
        defer( arena.reset() );

        wl_display_flush(pDisplay);

        const int pollStatus = poll(aFds, utils::size(aFds), updateRateMS);

        if (aFds[0].revents & POLLIN)
            if (wl_display_dispatch(pDisplay) == -1)
                return;

        constexpr int NO_SIGNAL = -2;
        int currSignalId = NO_SIGNAL;

        if (aFds[1].revents & POLLIN)
        {
            signalfd_siginfo fdsi;
            ssize_t n = read(fdSignals, &fdsi, sizeof(fdsi));

            if (n == sizeof(fdsi))
            {
                currSignalId = fdsi.ssi_signo - SIGRTMIN;
                LOG_GOOD("currSignalId: {}\n", currSignalId);
                g_bRedraw = true;
            }
        }

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
                            const int maxAbsX = 9999999
                        ) -> int
                    {
                        int thisXOff = 0;

                        auto fg = reinterpret_cast<const ImagePixelARGBle&>(fgColor);

                        for (const wchar_t ch : StringWCharIt(sv))
                        {
                            defer( thisXOff += xMove );
    
                            if (ch == L' ') continue;
                            if (ch == L'\n') break;

                            MapResult<u32, ttf::Rasterizer::UV> mFoundUV {};

                            try
                            {
                                mFoundUV = rRast.addOrSearchGlyph(
                                    &app::g_threadPool.scratchBuffer(),
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
    
                            const Span2D<u8> spAtlas = rRast.atlasSpan();
                            const int maxx = utils::min(utils::min(rBar.m_width, maxAbsX), xOffset + thisXOff + xScale);
                            for (int y = 0; y < yScale; ++y)
                            {
                                for (int x = xOffset + thisXOff; x < maxx; ++x)
                                {
                                    if (x < 0) break;

                                    const u8 val = spAtlas((x - xOffset - thisXOff) + u, y + v);
                                    if (val == 0) continue;

                                    auto& rDest = reinterpret_cast<ImagePixelARGBle&>(spBuffer(
                                        x, rBar.m_height - 1 - y - yOff
                                    ));

                                    const f32 t = val / 255.0f;

                                    rDest.a = 0xff;
                                    rDest.r = u8(math::lerp(rDest.r, fg.r, t));
                                    rDest.g = u8(math::lerp(rDest.g, fg.g, t));
                                    rDest.b = u8(math::lerp(rDest.b, fg.b, t));
                                }
                            }
                        }

                        return sv.size() * xMove;
                    };

                    int xOffStatus = rBar.m_width;
                    try
                    {
                        auto clDrawEntry = [&](const StringView sv, const int offset)
                        {
                            clDrawString(offset, sv, config::inl_colorScheme.status.fg);
                        };

                        Vec<Pair<StringView, int>> vEntryStrings {&arena};

                        auto clProcEntry = [&](config::StatusEntry* p, auto clWrite)
                        {
                            bool bTimedOut = p->updateRateMS > 0 && p->lastUpdateTimeMS + p->updateRateMS <= currTime;
                            bool bSignaled = p->signalId == currSignalId;

                            if (bTimedOut || bSignaled)
                            {
                                p->sfHolder = clWrite();
                                p->lastUpdateTimeMS = currTime;
                            }

                            xOffStatus -= p->sfHolder.size()*xMove;
                            vEntryStrings.emplace(&arena, p->sfHolder, xOffStatus);
                        };

                        const isize last = utils::size(config::g_aStatusEntries) - 1;
                        for (isize i = last; i >= 0; --i)
                        {
                            config::StatusEntry& entry = config::g_aStatusEntries[i];

                            switch (entry.eType)
                            {
                                case config::StatusEntry::TYPE::EXEC:
                                {
                                    clProcEntry(&entry, [&] { return exec::runReadOutput(entry); });
                                }
                                break;

                                case config::StatusEntry::TYPE::TEXT:
                                {
                                    if (entry.nts != nullptr)
                                    {
                                        xOffStatus -= strlen(entry.nts) * xMove;
                                        vEntryStrings.emplace(&arena, entry.nts, xOffStatus);
                                    }
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

                                    clProcEntry(&entry, clWrite);
                                }
                                break;

                                case config::StatusEntry::TYPE::KEYBOARD_LAYOUT:
                                {
                                    xOffStatus -= rBar.m_sfKbLayout.size() * xMove;
                                    vEntryStrings.emplace(&arena, rBar.m_sfKbLayout, xOffStatus);
                                }
                                break;

                                case config::StatusEntry::TYPE::FILE_WATCH:
                                {
                                    auto clWrite = [&]
                                    {
                                        config::String64 sf = file::load<config::String64::CAP>(entry.nts);
                                        if (entry.func.pfnFormatString)
                                            sf = entry.func.pfnFormatString(sf.data());

                                        return sf;
                                    };

                                    clProcEntry(&entry, clWrite);
                                }
                                break;

                                case config::StatusEntry::TYPE::BATTERY:
                                {
                                    auto clWrite = [&]
                                    {
                                        config::String64 sf {};
                                        battery::Report report = battery::Report::read(
                                            entry.nts, &app::g_threadPool.scratchBuffer()
                                        );

                                        if (entry.func.pfnFormatBattery)
                                            sf = entry.func.pfnFormatBattery(report);

                                        return sf;
                                    };

                                    clProcEntry(&entry, clWrite);
                                }
                                break;
                            }

                            xOffStatus -= xMove*2;
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
                        const isize tagI = rBar.m_vTags.idx(&tag);
                        char aTagStringBuff[4] {};
                        const isize n = print::toSpan(aTagStringBuff, "{}", 1 + tagI);
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

                        /* tag bg */
                        fillBg(spBuffer, tagXBegin, 0, xMove + xMove*n + xMove, yScale, bgColor);

                        if (tag.nClients > 0)
                        {
                            /* lil square */
                            const int height = rBar.m_height / 5;
                            const int yOff2 = height / 1.5;

                            /* xMove/4 further */
                            if (tag.eState == ZDWL_IPC_OUTPUT_V2_TAG_STATE_ACTIVE)
                                fillBg(spBuffer, xOff + xMove/4, yOff2, height*1.3, height*1.3, fgColor);
                            else fillBgOutline(spBuffer, xOff + xMove/4, yOff2, height*1.3, height*1.3, 1, fgColor);
                        }

                        xOff += xMove;
                        xOff += clDrawString(xOff, StringView {aTagStringBuff, n}, fgColor, xOffStatus);
                        xOff += xMove;
    
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

                    /* layout icon bg */
                    fillBg(spBuffer, xOff, 0, rBar.m_sfLayoutIcon.size()*xMove + xMove*2, yScale, config::inl_colorScheme.tag.bg);

                    xOff += xMove;
                    xOff += clDrawString(xOff, rBar.m_sfLayoutIcon, config::inl_colorScheme.status.fg, xOffStatus);
                    xOff += xMove;

                    /* title bg */
                    fillBg(spBuffer, xOff, 0, (xOffStatus - xOff) + xMove, yScale, config::inl_colorScheme.title.bg);

                    xOff += xMove;
                    xOff += clDrawString(xOff, rBar.m_sfTitle, config::inl_colorScheme.title.fg, xOffStatus);
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
