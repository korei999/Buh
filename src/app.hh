#pragma once

#include "ttf/Font.hh"
#include "ttf/Rasterizer.hh"
#include "wayland/Client.hh"
#include "PipeWire/PipeWire.hh"

#include "adt/ScratchBuffer.hh"
#include "adt/ThreadPool.hh"

namespace app
{

extern int g_argc;
extern const char* const* g_argv;

extern bool g_bRunning;

extern wayland::Client g_client;
extern PipeWire g_pw;

extern ttf::Font g_font;
extern ttf::Rasterizer g_rasterizer;

extern thread_local adt::ScratchBuffer gtl_scratch;
extern adt::ThreadPool<128> g_threadPool;

void allocScratchForThisThread(adt::ssize size);
void destroyScratchForThisThread();

} /* namespace app */
