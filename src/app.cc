#include "app.hh"

#include "adt/StdAllocator.hh"

using namespace adt;

namespace app
{

int g_argc {};
const char* const* g_argv {};
bool g_bRunning {};
wl::Client g_client {};
ttf::Font g_font {};
ttf::Rasterizer g_rasterizer {};

static thread_local u8* stl_pScratchMem;
thread_local ScratchBuffer gtl_scratch;

ThreadPool<128> g_threadPool {};

void
allocScratchForThisThread(ssize size)
{
    ADT_ASSERT(stl_pScratchMem == nullptr, "already allocated");

    stl_pScratchMem = StdAllocator::inst()->zallocV<u8>(size);
    gtl_scratch = ScratchBuffer {stl_pScratchMem, size};
}

void
destroyScratchForThisThread()
{
    StdAllocator::inst()->free(stl_pScratchMem);
    stl_pScratchMem = {};
    gtl_scratch = {};
}

} /* namespace app */
