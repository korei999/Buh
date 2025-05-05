#include "app.hh"
#include "frame.hh"

#include "adt/defer.hh"
#include "adt/StdAllocator.hh"

#include "font.bin"

#include <string.h>

using namespace adt;

static int s_barHeight = 24;

static void
parseArgs(const int argc, const char* const* argv)
{
    for (int i = 1; i < argc; ++i)
    {
        const StringView sv = argv[i];

        if (sv.beginsWith("--"))
        {
            if (sv == "--height")
            {
                if (i + 1 < argc)
                {
                    ++i;
                    int num = atoi(argv[i]);
                    if (num == 0)
                    {
                        print::err("failed to parse the height number\n");
                        exit(1);
                    }
                    s_barHeight = num;
                }
            }
        }
        else return;
    }
}

int
main(const int argc, const char* const* argv)
{
    app::g_argc = argc, app::g_argv = argv;

    parseArgs(argc, argv);

    new(&app::g_threadPool) ThreadPool<128> {StdAllocator::inst(),
        +[](void*) { app::allocScratchForThisThread(SIZE_1M); }, {},
        +[](void*) { app::destroyScratchForThisThread(); }, {},
        utils::max(ADT_GET_NPROCS() - 1, 2)
    };

    app::g_threadPool.start();
    defer( app::g_threadPool.destroy(StdAllocator::inst()) );

    app::allocScratchForThisThread(SIZE_1M);
    defer( app::destroyScratchForThisThread() );

    if (!bool(app::g_font = ttf::Font {StdAllocator::inst(), StringView {(char*)LiberationMono_Regular_ttf, LiberationMono_Regular_ttf_len}}))
    {
        print::err("failed to parse the font\n");
        return 1;
    }

    app::g_rasterizer.rasterizeAscii(StdAllocator::inst(), &app::g_font, s_barHeight);

    wl::Client client {"Buh", s_barHeight};
    app::g_pClient = &client;
    defer( app::client().destroy() );

    app::g_bRunning = true;

    frame::run();
}
