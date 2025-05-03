#include "app.hh"
#include "frame.hh"

#include "adt/defer.hh"
#include "adt/file.hh"
#include "adt/StdAllocator.hh"

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

    file::Mapped fmFont = file::map("LiberationMono-Regular.ttf");
    if (!fmFont)
    {
        print::err("failed to load the font\n");
        return 1;
    }

    if (!bool(app::g_font = ttf::Font {StdAllocator::inst(), fmFont}))
    {
        print::err("failed to parse the font\n");
        return 1;
    }
    fmFont.unmap();

    app::g_rasterizer.rasterizeAscii(StdAllocator::inst(), &app::g_font, 24);

    wl::Client client {"Buh", 24};
    app::g_pClient = &client;
    defer( app::client().destroy() );

    app::g_bRunning = true;

    frame::run();
}
