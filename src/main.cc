#include "app.hh"
#include "frame.hh"

#include "adt/defer.hh"
#include "adt/StdAllocator.hh"

#include "font.bin"

using namespace adt;

int
main()
{
    app::g_threadPool = ThreadPool<128> {StdAllocator::inst(),
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

    const int scale = 24;
    app::g_rasterizer.rasterizeAscii(StdAllocator::inst(), &app::g_font, scale);

    wl::Client client {"Buh", scale};
    app::g_pClient = &client;
    defer( app::client().destroy() );

    app::g_bRunning = true;

    frame::run();
}
