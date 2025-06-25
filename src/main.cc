#include "app.hh"
#include "frame.hh"

#include "font.bin"

#include "adt/file.hh"
#include "adt/StdAllocator.hh"

#include <clocale>

#include "config.hh"

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
                    int num = atoi(argv[++i]);
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
                    s_ntsFontPath = argv[++i];
                }
                else
                {
                    print::err("failed to parse font arg\n");
                    exit(1);
                }
            }
            else if (sv == "--preset")
            {
                if (i + 1 < argc)
                {
                    int num = atoi(argv[++i]);
                    if (num < 0 || num >= utils::size(config::aColorSchemes))
                    {
                        print::err("index must be '>= 0 && < {}', got: '{}'\n", utils::size(config::aColorSchemes), num);
                        exit(1);
                    }
                    config::inl_colorScheme = config::aColorSchemes[num];
                }
                else
                {
                    print::err("failed to parse color scheme index\n");
                    exit(1);
                }
            }
        }
        else return;
    }
}

static int
startup()
{
    parseArgs(app::g_argc, app::g_argv);

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

    defer( app::g_font.destroy() );

    app::g_rasterizer.rasterizeAscii(StdAllocator::inst(), &app::g_font, &app::g_threadPool, s_barHeight);
    defer( app::g_rasterizer.destroy(StdAllocator::inst()) );

    new(&app::g_wlClient) wayland::Client {"Buh", s_barHeight};
    defer( app::g_wlClient.destroy() );

    app::g_bRunning = true;

    /* no need in the pool anymore */
    app::g_threadPool.destroyKeepScratchBuffer(StdAllocator::inst());

    frame::run();

    app::g_threadPool.destroyScratchBufferForThisThread();

    return 0;
}

int
main(const int argc, const char* const* argv)
{
    app::g_argc = argc, app::g_argv = argv;

    setlocale(LC_ALL, "");

    try
    {
        startup();
    }
    catch (const IException& ex)
    {
        ex.printErrorMsg(stderr);
    }
}
