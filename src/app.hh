#pragma once

#include "ttf/Parser.hh"
#include "ttf/Rasterizer.hh"
#include "wayland/Client.hh"

#include "adt/ThreadPool.hh"

namespace app
{

extern int g_argc;
extern const char* const* g_argv;

extern bool g_bRunning;

extern wayland::Client g_wlClient;

extern ttf::Parser g_font;
extern ttf::Rasterizer g_rasterizer;

extern adt::ThreadPoolWithMemory<128> g_threadPool;

} /* namespace app */
