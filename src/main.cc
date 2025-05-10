#include "app.hh"
#include "frame.hh"

#include "font.bin"

#include "adt/file.hh"
#include "adt/StdAllocator.hh"

#include <clocale>

using namespace adt;

static int s_barHeight = 24;
static const char* s_ntsFontPath {};

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
                        print::err("height must be '> 0', got: '{}'\n", num);
                        exit(1);
                    }
                    s_barHeight = num;
                }
            }
            else if (sv == "--font")
            {
                if (i + 1 < argc)
                {
                    ++i;
                    s_ntsFontPath = argv[i];
                }
                else
                {
                    print::err("failed to parse font arg\n");
                    exit(1);
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

    setlocale(LC_ALL, "");

    parseArgs(argc, argv);

    new(&app::g_threadPool) ThreadPoolWithMemory<128> {StdAllocator::inst(), SIZE_1M};

    String sFile {};
    defer( sFile.destroy(StdAllocator::inst()) );

    if (!s_ntsFontPath)
    {
        new(&app::g_font) ttf::Parser {StdAllocator::inst(), StringView {
            (char*)LiberationMono_Regular_ttf, LiberationMono_Regular_ttf_len
        }};

        if (!app::g_font)
        {
            print::err("failed to parse the font\n");
            return 1;
        }
    }
    else
    {
        sFile = file::load(StdAllocator::inst(), s_ntsFontPath);
        if (!sFile)
        {
            print::err("failed to load file '{}'\n", s_ntsFontPath);
            return 1;
        }

        new(&app::g_font) ttf::Parser {StdAllocator::inst(), sFile};
        if (!app::g_font)
        {
            print::err("failed to parse the font\n");
            return 1;
        }
    }

    app::g_rasterizer.rasterizeAscii(StdAllocator::inst(), &app::g_font, &app::g_threadPool, s_barHeight);
    defer( app::g_rasterizer.destroy(StdAllocator::inst()) );

    new(&app::g_wlClient) wayland::Client {"Buh", s_barHeight};
    defer( app::g_wlClient.destroy() );

    app::g_bRunning = true;

    /* no need it the pool anymore */
    app::g_threadPool.destroyKeepScratch(StdAllocator::inst());
    frame::run();
}
