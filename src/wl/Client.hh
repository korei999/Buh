#pragma once

#include "WaylandGlueGenerated/wlr-layer-shell.h"
#include "WaylandGlueGenerated/dwl-ipc.h"

#include "Tag.hh"

#include "adt/Vec.hh"

namespace wl
{

struct Client
{
    struct Bar
    {
        Client* m_pClient {};
        wl_output* m_pOutput {};
        wl_surface* m_pSurface {};
        zwlr_layer_surface_v1* m_pLayerSurface {};
        wl_buffer* m_pBuffer {};
        wl_buffer* m_pBufferHidden {};
        zdwl_ipc_output_v2* m_pDwlOutput {};

        wl_shm_pool* m_pShmPool {};
        adt::u8* m_pPoolData {};
        adt::ssize m_poolSize {};

        int m_width {};
        int m_height {};
        int m_stride {};
        adt::u32 m_outputRegistryName {};

        adt::StringFixed<128> m_sfTitle {};
        adt::StringFixed<128> m_sfAppid {};
        adt::StringFixed<8> m_sfLayoutIcon {};
        adt::StringFixed<32> m_sfKbLayout {};

        adt::Vec<Tag> m_vTags {};

        bool m_bHidden {};

        /* */

        void allocShmBuffer();
        void destroyShmBuffer();
        void destroy();

        void toggleVisibility(zdwl_ipc_output_v2* zdwl_ipc_output_v2);
        void active(zdwl_ipc_output_v2* zdwl_ipc_output_v2, adt::u32 active);
        void tag(zdwl_ipc_output_v2* zdwl_ipc_output_v2, adt::u32 tag, adt::u32 state, adt::u32 clients, adt::u32 focused);
        void layout(zdwl_ipc_output_v2* zdwl_ipc_output_v2, adt::u32 layout); /* tile/monocle etc... */
        void title(zdwl_ipc_output_v2* zdwl_ipc_output_v2, const char* title);
        void appid(zdwl_ipc_output_v2* zdwl_ipc_output_v2, const char* appid);
        void layoutSymbol(zdwl_ipc_output_v2* zdwl_ipc_output_v2, const char* layout);
        void keyboardLayout(zdwl_ipc_output_v2* zdwl_ipc_output_v2, const char* kblayout);
        void frame(zdwl_ipc_output_v2* zdwl_ipc_output_v2);
        void fullscreen(zdwl_ipc_output_v2* zdwl_ipc_output_v2, adt::u32 is_fullscreen);
        void floating(zdwl_ipc_output_v2* zdwl_ipc_output_v2, adt::u32 is_floating);
    };

    struct Pointer
    {
        enum class BUTTON : adt::u32
        {
            MOUSE = 0x110,
            LEFT = 0x110,
            RIGHT = 0x111,
            MIDDLE = 0x112,
            SIDE = 0x113,
            EXTRA = 0x114,
            FORWARD = 0x115,
            BACK = 0x116,
            TASK = 0x117,
        };

        wl_surface* pLastEnterSufrace {};
        adt::f64 surfacePointerX {};
        adt::f64 surfacePointerY {};
        adt::u32 time {};
        BUTTON eButton {};
        adt::u32 state {};
    };

    /* */

    adt::StringFixed<32> m_sfName {};

    wl_display* m_pDisplay {};
    wl_registry* m_pRegistry {};
    wl_compositor* m_pCompositor {};

    adt::Vec<Bar> m_vBars {};

    wl_shm* m_pShm {};
    int m_barHeight {};

    wl_seat* m_pSeat {};
    wl_pointer* m_pPointer {};

    zwlr_layer_shell_v1* m_pLayerShell {};
    zwlr_layer_surface_v1* m_pLayerSurface {};

    zdwl_ipc_manager_v2* m_pDwlManager {};

    Pointer m_pointer {};

    /* */

    Client() = default;
    Client(const char* ntsName, const int height);

    /* */

    void destroy();

    void registryGlobal(wl_registry* wl_registry, adt::u32 name, const char* interface, adt::u32 version);
    void registryGlobalRemove(wl_registry* wl_registry, adt::u32 name);

    void outputGeometry(
        wl_output* wl_output, adt::i32 x, adt::i32 y, adt::i32 physical_width, adt::i32 physical_height,
        adt::i32 subpixel, const char* make, const char* model, adt::i32 transform
    );
    void outputMode(wl_output* wl_output, adt::u32 flags, adt::i32 width, adt::i32 height, adt::i32 refresh);
    void outputDone(wl_output* wl_output);
    void outputScale(wl_output* wl_output, adt::i32 factor);
    void outputName(wl_output* wl_output, const char* name);
    void outputDescription(wl_output* wl_output, const char* description);

    void seatCapabilities(wl_seat* wl_seat, adt::u32 capabilities);
    void seatName(wl_seat* wl_seat, const char* name);

    void pointerEnter(wl_pointer* wl_pointer, adt::u32 serial, wl_surface* surface, wl_fixed_t surface_x, wl_fixed_t surface_y);
    void pointerLeave(wl_pointer* wl_pointer, adt::u32 serial, wl_surface* surface);
    void pointerMotion(wl_pointer* wl_pointer, adt::u32 time, wl_fixed_t surface_x, wl_fixed_t surface_y);
    void pointerButton(wl_pointer* wl_pointer, adt::u32 serial, adt::u32 time, adt::u32 button, adt::u32 state);
    void pointerAxis(wl_pointer* wl_pointer, adt::u32 time, adt::u32 axis, wl_fixed_t value);
	void pointerFrame(wl_pointer *wl_pointer);

    void dwlTags(zdwl_ipc_manager_v2* zdwl_ipc_manager_v2, adt::u32 amount);
    void dwlLayout(zdwl_ipc_manager_v2* zdwl_ipc_manager_v2, const char* name);
};

} /* namespace wl */
