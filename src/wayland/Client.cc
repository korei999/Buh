#include "Client.hh"

using namespace adt;

namespace wayland
{

#if defined __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#if defined __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

static void
layerSurfaceConfigure(
    void* p,
    zwlr_layer_surface_v1* zwlr_layer_surface_v1,
    u32 serial,
    u32 width,
    u32 height
)
{
    ADT_ASSERT(p != nullptr, "p: {}\n", p);
    LOG_WARN("width/height: [{}], surface: {}\n", Pair {width, height}, zwlr_layer_surface_v1);

    Client::Bar& bar = *static_cast<Client::Bar*>(p);
    ADT_ASSERT(bar.m_pLayerSurface == zwlr_layer_surface_v1, "");

    if (bar.m_pPoolData == nullptr)
    {
        bar.m_width = width;
        bar.m_height = height;
        bar.allocShmBuffer();
    }
    else if (bar.m_width != static_cast<int>(width) || bar.m_height != static_cast<int>(height))
    {
        bar.destroyShmBuffer();
        bar.m_width = width;
        bar.m_height = height;
        bar.allocShmBuffer();
    }

    zwlr_layer_surface_v1_ack_configure(zwlr_layer_surface_v1, serial);
}

static void
layerSurfaceClosed(
    [[maybe_unused]] void* p,
    [[maybe_unused]] zwlr_layer_surface_v1* zwlr_layer_surface_v1
)
{
}

static const wl_registry_listener s_registryListener {
    .global = decltype(wl_registry_listener::global)(methodPointer(&Client::registryGlobal)),
    .global_remove = decltype(wl_registry_listener::global_remove)(methodPointer(&Client::registryGlobalRemove)),
};

static const wl_output_listener s_outputListener {
    .geometry = decltype(wl_output_listener::geometry)(methodPointer(&Client::outputGeometry)),
    .mode = decltype(wl_output_listener::mode)(methodPointer(&Client::outputMode)),
    .done = decltype(wl_output_listener::done)(methodPointer(&Client::outputDone)),
    .scale = decltype(wl_output_listener::scale)(methodPointer(&Client::outputScale)),
    .name = decltype(wl_output_listener::name)(methodPointer(&Client::outputName)),
    .description = decltype(wl_output_listener::description)(methodPointer(&Client::outputDescription)),
};

static const wl_seat_listener s_seatListener {
    .capabilities = decltype(wl_seat_listener::capabilities)(methodPointer(&Client::seatCapabilities)),
    .name = decltype(wl_seat_listener::name)(methodPointer(&Client::seatName)),
};

static zwlr_layer_surface_v1_listener s_layerSurfaceListener {
    .configure = layerSurfaceConfigure,
    .closed = layerSurfaceClosed,
};

static const wl_pointer_listener s_pointerListener {
    .enter = decltype(wl_pointer_listener::enter)(methodPointer(&Client::pointerEnter)),
    .leave = decltype(wl_pointer_listener::leave)(methodPointer(&Client::pointerLeave)),
    .motion = decltype(wl_pointer_listener::motion)(methodPointer(&Client::pointerMotion)),
    .button = decltype(wl_pointer_listener::button)(methodPointer(&Client::pointerButton)),
    .axis = decltype(wl_pointer_listener::axis)(methodPointer(&Client::pointerAxis)),
    .frame = decltype(wl_pointer_listener::frame)(methodPointer(&Client::pointerFrame)),
};

static const zdwl_ipc_manager_v2_listener s_dwlListener {
    .tags = decltype(zdwl_ipc_manager_v2_listener::tags)(methodPointer(&Client::dwlTags)),
    .layout = decltype(zdwl_ipc_manager_v2_listener::layout)(methodPointer(&Client::dwlLayout)),
};

static const zdwl_ipc_output_v2_listener s_dwlOutputListener {
    .toggle_visibility = decltype(zdwl_ipc_output_v2_listener::toggle_visibility)(methodPointer(&Client::Bar::toggleVisibility)),
    .active = decltype(zdwl_ipc_output_v2_listener::active)(methodPointer(&Client::Bar::active)),
    .tag = decltype(zdwl_ipc_output_v2_listener::tag)(methodPointer(&Client::Bar::tag)),
    .layout = decltype(zdwl_ipc_output_v2_listener::layout)(methodPointer(&Client::Bar::layout)),
    .title = decltype(zdwl_ipc_output_v2_listener::title)(methodPointer(&Client::Bar::title)),
    .appid = decltype(zdwl_ipc_output_v2_listener::appid)(methodPointer(&Client::Bar::appid)),
    .layout_symbol = decltype(zdwl_ipc_output_v2_listener::layout_symbol)(methodPointer(&Client::Bar::layoutSymbol)),
    .kblayout = decltype(zdwl_ipc_output_v2_listener::kblayout)(methodPointer(&Client::Bar::keyboardLayout)),
    .frame = decltype(zdwl_ipc_output_v2_listener::frame)(methodPointer(&Client::Bar::frame)),
    .fullscreen = decltype(zdwl_ipc_output_v2_listener::fullscreen)(methodPointer(&Client::Bar::fullscreen)),
    .floating = decltype(zdwl_ipc_output_v2_listener::floating)(methodPointer(&Client::Bar::floating)),
};

#if defined __clang__
    #pragma clang diagnostic pop
#endif

#if defined __GNUC__
    #pragma GCC diagnostic pop
#endif

Client::Client(const char* ntsName, const int height)
    : m_sfName {ntsName}, m_barHeight {height}
{
    ADT_ASSERT_ALWAYS(m_pDisplay = wl_display_connect({}), "");
    ADT_ASSERT_ALWAYS(m_pRegistry = wl_display_get_registry(m_pDisplay), "");

    wl_registry_add_listener(m_pRegistry, &s_registryListener, this);
    wl_display_roundtrip(m_pDisplay);
}

void
Client::destroy()
{
    LOG_WARN("destroy()...\n");

    for (auto& bar : m_vBars) bar.destroy();
    m_vBars.destroy(StdAllocator::inst());

    if (m_pDwlManager) zdwl_ipc_manager_v2_destroy(m_pDwlManager);
    if (m_pLayerShell) zwlr_layer_shell_v1_destroy(m_pLayerShell);
    if (m_pPointer) wl_pointer_destroy(m_pPointer);
    if (m_pSeat) wl_seat_destroy(m_pSeat);
    if (m_pShm) wl_shm_destroy(m_pShm);
    if (m_pCompositor) wl_compositor_destroy(m_pCompositor);
    if (m_pRegistry) wl_registry_destroy(m_pRegistry);
    if (m_pDisplay) wl_display_disconnect(m_pDisplay);
}

void
Client::registryGlobal(
    wl_registry* wl_registry,
    u32 name,
    const char* interface,
    u32 version
)
{
    StringView svInterface = StringView(interface);

    if (svInterface == wl_compositor_interface.name)
    {
        ADT_ASSERT_ALWAYS(
            m_pCompositor = static_cast<wl_compositor*>(
                wl_registry_bind(wl_registry, name, &wl_compositor_interface, version)
            ),
            ""
        );
        LOG_GOOD("interface: '{}', version: {}, name: {}\n", interface, version, name);
    }
    else if (svInterface == wl_seat_interface.name)
    {
        ADT_ASSERT_ALWAYS(
            m_pSeat = static_cast<wl_seat*>(
                wl_registry_bind(wl_registry, name, &wl_seat_interface, 4)
            ),
            ""
        );
        LOG_GOOD("interface: '{}', version: {}, name: {}\n", interface, version, name);

        wl_seat_add_listener(m_pSeat, &s_seatListener, this);
    }
    else if (svInterface == zwlr_layer_shell_v1_interface.name)
    {
        ADT_ASSERT_ALWAYS(
            m_pLayerShell = static_cast<zwlr_layer_shell_v1*>(
                wl_registry_bind(wl_registry, name, &zwlr_layer_shell_v1_interface, version)
            ),
            ""
        );
        LOG_GOOD("interface: '{}', version: {}, name: {}\n", interface, version, name);
    }
    else if (svInterface == wl_output_interface.name)
    {
        wl_output* p {};
        ADT_ASSERT_ALWAYS(
            p = static_cast<wl_output*>(
                wl_registry_bind(wl_registry, name, &wl_output_interface, version)
            ),
            ""
        );
        LOG_GOOD("interface: '{}', version: {}, name: {}, registry: {}\n", interface, version, name, wl_registry);

        auto* pSurface = wl_compositor_create_surface(m_pCompositor);
        ADT_ASSERT_ALWAYS(pSurface, "wl_compositor_create_surface(m_pCompositor) failed");

        m_vBars.push(StdAllocator::inst(), {});

        {
            auto& bar = m_vBars.last();

            bar.m_pOutput = p;
            bar.m_pSurface = pSurface;
            bar.m_outputRegistryName = name;

            bar.m_pClient = this;

            bar.m_pLayerSurface = zwlr_layer_shell_v1_get_layer_surface(
                m_pLayerShell,
                bar.m_pSurface,
                bar.m_pOutput,
                ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM,
                m_sfName.data()
            );

            zwlr_layer_surface_v1_add_listener(bar.m_pLayerSurface, &s_layerSurfaceListener, &bar);
            zwlr_layer_surface_v1_set_size(bar.m_pLayerSurface, 0, m_barHeight);
            zwlr_layer_surface_v1_set_anchor(bar.m_pLayerSurface,
                ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT
            );
            zwlr_layer_surface_v1_set_exclusive_zone(bar.m_pLayerSurface, m_barHeight);

            ADT_ASSERT_ALWAYS(
                bar.m_pDwlOutput = zdwl_ipc_manager_v2_get_output(m_pDwlManager, bar.m_pOutput),
                "Failed to get dwl output"
            );

            zdwl_ipc_output_v2_add_listener(bar.m_pDwlOutput, &s_dwlOutputListener, &bar);

            wl_surface_commit(bar.m_pSurface);
            wl_display_dispatch(m_pDisplay);
        }

        wl_output_add_listener(p, &s_outputListener, this);
    }
    else if (svInterface == wl_shm_interface.name)
    {
        ADT_ASSERT_ALWAYS(
            m_pShm = static_cast<wl_shm*>(
                wl_registry_bind(wl_registry, name, &wl_shm_interface, version)
            ),
            ""
        );
        LOG_GOOD("interface: '{}', version: {}, name: {}\n", interface, version, name);
    }
    else if (svInterface == zdwl_ipc_manager_v2_interface.name)
    {
        ADT_ASSERT_ALWAYS(
            m_pDwlManager = static_cast<zdwl_ipc_manager_v2*>(
                wl_registry_bind(wl_registry, name, &zdwl_ipc_manager_v2_interface, version)
            ),
            ""
        );
        LOG_GOOD("interface: '{}', version: {}, name: {}\n", interface, version, name);

        zdwl_ipc_manager_v2_add_listener(m_pDwlManager, &s_dwlListener, this);
    }
}

void
Client::registryGlobalRemove(
    [[maybe_unused]] wl_registry* wl_registry,
    [[maybe_unused]] u32 name
)
{
    for (ssize i = 0; i < m_vBars.size(); ++i)
    {
        Bar& bar = m_vBars[i];
        if (name == bar.m_outputRegistryName)
        {
            bar.destroy();
            m_vBars.popAsLast(i);
        }
    }
}

void
Client::outputGeometry(
    [[maybe_unused]] wl_output* wl_output,
    [[maybe_unused]] i32 x,
    [[maybe_unused]] i32 y,
    [[maybe_unused]] i32 physical_width,
    [[maybe_unused]] i32 physical_height,
    [[maybe_unused]] i32 subpixel,
    [[maybe_unused]] const char* make,
    [[maybe_unused]] const char* model,
    [[maybe_unused]] i32 transform
)
{
}

void
Client::outputMode(
    [[maybe_unused]] wl_output* wl_output,
    [[maybe_unused]] u32 flags,
    [[maybe_unused]] i32 width,
    [[maybe_unused]] i32 height,
    [[maybe_unused]] i32 refresh
)
{
    LOG_NOTIFY(
        "(mode): output: '{}', flags: '{}', width: '{}', height: '{}', refresh: '{}'\n",
        wl_output, flags, width, height, refresh
    );
}

void
Client::outputDone([[maybe_unused]] wl_output* wl_output)
{
}

void
Client::outputScale(
    [[maybe_unused]] wl_output* wl_output,
    [[maybe_unused]] i32 factor
)
{
}

void
Client::outputName(
    [[maybe_unused]] wl_output* wl_output,
    [[maybe_unused]] const char* name
)
{
    LOG_NOTIFY("name: '{}'\n", name);
}

void
Client::outputDescription(
    [[maybe_unused]] wl_output* wl_output,
    [[maybe_unused]] const char* description
)
{
    LOG_NOTIFY("{}\n", description);
}

void
Client::seatCapabilities(
    wl_seat* wl_seat,
    u32 capabilities
)
{
    if (capabilities & WL_SEAT_CAPABILITY_POINTER)
    {
        m_pPointer = wl_seat_get_pointer(wl_seat);
        wl_pointer_add_listener(m_pPointer, &s_pointerListener, this);
    }
}

void
Client::seatName(
    [[maybe_unused]] wl_seat* wl_seat,
    [[maybe_unused]] const char* name
)
{
}

void
Client::dwlTags(
    [[maybe_unused]] zdwl_ipc_manager_v2* zdwl_ipc_manager_v2,
    [[maybe_unused]] u32 amount
)
{
}

void
Client::dwlLayout(
    [[maybe_unused]] zdwl_ipc_manager_v2* zdwl_ipc_manager_v2,
    [[maybe_unused]] const char* name
)
{
    LOG_BAD("({}), name: '{}'\n", zdwl_ipc_manager_v2, name);
}

} /* namespace wayland */
