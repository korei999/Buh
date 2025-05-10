#include "app.hh"

using namespace adt;

namespace app
{

int g_argc {};
const char* const* g_argv {};

bool g_bRunning {};

wayland::Client g_wlClient {};

ttf::Parser g_font {};
ttf::Rasterizer g_rasterizer {};

ThreadPoolWithMemory<128> g_threadPool {};

} /* namespace app */
