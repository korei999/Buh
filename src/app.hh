#pragma once

#include "ttf/Font.hh"
#include "ttf/Rasterizer.hh"
#include "wl/Client.hh"

#include "adt/ScratchBuffer.hh"
#include "adt/ThreadPool.hh"

namespace app
{

extern bool g_bRunning;
extern wl::Client* g_pClient;
extern ttf::Font g_font;
extern ttf::Rasterizer g_rasterizer;
inline wl::Client& client() { return *g_pClient; }

extern thread_local adt::ScratchBuffer gtl_scratch;
extern adt::ThreadPool<128> g_threadPool;

void allocScratchForThisThread(adt::ssize size);
void destroyScratchForThisThread();

} /* namespace app */
