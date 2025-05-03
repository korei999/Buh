#pragma once

#include "wayland-protocols/wlr-layer-shell.h"
#include "wayland-protocols/dwl-ipc.h"

#include "Tag.hh"

#include "adt/Vec.hh"

namespace wl
{

struct Client
{
    struct BarOutput
    {
        Client* m_pClient {};
        wl_output* m_pOutput {};
        wl_surface* m_pSurface {};
        zwlr_layer_surface_v1* m_pLayerSurface {};
        wl_buffer* m_pBuffer {};
        zdwl_ipc_output_v2* m_pDwlOutput {};

        int m_width {};
        int m_height {};
        int m_stride {};

        adt::StringFixed<128> m_sfTitle {};
        adt::StringFixed<128> m_sfAppid {};
        adt::StringFixed<8> m_sfLayoutIcon {};
        adt::StringFixed<32> m_sfKbLayout {};

        adt::Vec<Tag> m_vTags {};

        /* */

        void createBuffer(int width, int height);

        void toggleVisibility(zdwl_ipc_output_v2* zdwl_ipc_output_v2);
        void active(zdwl_ipc_output_v2* zdwl_ipc_output_v2, uint32_t active);
        void tag(zdwl_ipc_output_v2* zdwl_ipc_output_v2, uint32_t tag, uint32_t state, uint32_t clients, uint32_t focused);
        void layout(zdwl_ipc_output_v2* zdwl_ipc_output_v2, uint32_t layout); /* tile/monocle etc... */
        void title(zdwl_ipc_output_v2* zdwl_ipc_output_v2, const char* title);
        void appid(zdwl_ipc_output_v2* zdwl_ipc_output_v2, const char* appid);
        void layoutSymbol(zdwl_ipc_output_v2* zdwl_ipc_output_v2, const char* layout);
        void keyboardLayout(zdwl_ipc_output_v2* zdwl_ipc_output_v2, const char* kblayout);
        void frame(zdwl_ipc_output_v2* zdwl_ipc_output_v2);
        void fullscreen(zdwl_ipc_output_v2* zdwl_ipc_output_v2, uint32_t is_fullscreen);
        void floating(zdwl_ipc_output_v2* zdwl_ipc_output_v2, uint32_t is_floating);
    };

    /* */

    wl_display* m_pDisplay {};
    wl_registry* m_pRegistry {};
    wl_compositor* m_pCompositor {};

    adt::Vec<BarOutput> m_vOutputBars {};

    wl_shm* m_pShm {};
    wl_shm_pool* m_pShmPool {};
    adt::u8* m_pPoolData {};
    adt::ssize m_poolSize {};
    int m_maxWidth {};
    int m_barHeight {};

    wl_seat* m_pSeat {};
    wl_pointer* m_pPointer {};
    wl_keyboard* m_pKeyboard {};

    zwlr_layer_shell_v1* m_pLayerShell {};
    zwlr_layer_surface_v1* m_pLayerSurface {};

    zdwl_ipc_manager_v2* m_pDwlManager {};

    /* */

    Client() = default;
    Client(const char* ntsName, const int height);

    /* */

    void destroy();

    void registryGlobal(wl_registry* wl_registry, adt::u32 name, const char* interface, adt::u32 version);
    void registryGlobalRemove(wl_registry* wl_registry, adt::u32 name);

	void shmFormat(wl_shm *wl_shm, adt::u32 format);

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

    void pointerEnter(wl_pointer* wl_pointer, uint32_t serial, wl_surface* surface, wl_fixed_t surface_x, wl_fixed_t surface_y);
    void pointerLeave(wl_pointer* wl_pointer, uint32_t serial, wl_surface* surface);
    void pointerMotion(wl_pointer* wl_pointer, uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y);
    void pointerButton(wl_pointer* wl_pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
    void pointerAxis(wl_pointer* wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value);
	void pointerFrame(wl_pointer *wl_pointer);

    void keyboardKeymap(wl_keyboard* wl_keyboard, uint32_t format, int32_t fd, uint32_t size);
    void keyboardEnter(wl_keyboard* wl_keyboard, uint32_t serial, wl_surface* surface, wl_array* keys);
    void keyboardLeave(wl_keyboard* wl_keyboard, uint32_t serial, wl_surface* surface);
    void keyboardKey(wl_keyboard* wl_keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
    void keyboardModifiers(wl_keyboard* wl_keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group);
    void keyboardRepeatInfo(wl_keyboard* wl_keyboard, int32_t rate, int32_t delay);

    void dwlTags(zdwl_ipc_manager_v2* zdwl_ipc_manager_v2, uint32_t amount);
    void dwlLayout(zdwl_ipc_manager_v2* zdwl_ipc_manager_v2, const char* name);

    void initPool();
};

} /* namespace wl */
