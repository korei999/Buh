#include "Client.hh"

#include "wl/shm.hh"
#include "frame.hh"

#include "adt/StdAllocator.hh"
#include "adt/defer.hh"
#include "adt/logs.hh"

using namespace adt;

namespace wl
{

void
Client::Bar::allocShmBuffer()
{
    const int stride = m_width * 4;
    const int shmPoolSize = (m_pClient->m_barHeight * stride);

    const int fd = shm::allocFile(shmPoolSize);
    defer( close(fd) );
    m_pPoolData = static_cast<u8*>(mmap(nullptr, shmPoolSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    m_poolSize = shmPoolSize;
    ADT_ASSERT_ALWAYS(m_pPoolData, "mmap() failed");

    m_pShmPool = wl_shm_create_pool(m_pClient->m_pShm, fd, shmPoolSize);
    ADT_ASSERT_ALWAYS(m_pShmPool, "wl_shm_create_pool() failed");

    ADT_ASSERT_ALWAYS(
        m_pBuffer = wl_shm_pool_create_buffer(
            m_pShmPool, 0, m_width, m_pClient->m_barHeight, m_width * 4, WL_SHM_FORMAT_ARGB8888
        ),
        ""
    );
}

void
Client::Bar::destroyShmBuffer()
{
    wl_shm_pool_destroy(m_pShmPool);
    munmap(m_pPoolData, m_poolSize);
    m_height = 0, m_width = 0, m_stride = 0;
}

void
Client::Bar::destroy()
{
    destroyShmBuffer();
    wl_output_destroy(m_pOutput);
    zwlr_layer_surface_v1_destroy(m_pLayerSurface);
    wl_surface_destroy(m_pSurface);
    zdwl_ipc_output_v2_destroy(m_pDwlOutput);
    wl_buffer_destroy(m_pBuffer);
}

void
Client::Bar::toggleVisibility([[maybe_unused]] zdwl_ipc_output_v2* zdwl_ipc_output_v2)
{
    utils::toggle(&m_bHidden);
    LOG_NOTIFY("toggleVisibility: {}\n", m_bHidden);
    // frame::g_bRedraw = true;
}

void
Client::Bar::active(
    [[maybe_unused]] zdwl_ipc_output_v2* zdwl_ipc_output_v2,
    [[maybe_unused]] u32 active
)
{
}

void
Client::Bar::tag(
    [[maybe_unused]] zdwl_ipc_output_v2* zdwl_ipc_output_v2,
    [[maybe_unused]] u32 tag,
    [[maybe_unused]] u32 state,
    [[maybe_unused]] u32 clients,
    [[maybe_unused]] u32 focused
)
{
    if (tag >= m_vTags.size())
        m_vTags.setSize(StdAllocator::inst(), tag + 1);

    m_vTags[tag] = {
        .eState = zdwl_ipc_output_v2_tag_state(state),
        .nClients = static_cast<int>(clients),
        .bFocused = static_cast<bool>(focused),
    };
}

void
Client::Bar::layout(
    [[maybe_unused]] zdwl_ipc_output_v2* zdwl_ipc_output_v2,
    [[maybe_unused]] u32 layout
)
{
}

void
Client::Bar::title(
    [[maybe_unused]] zdwl_ipc_output_v2* zdwl_ipc_output_v2,
    [[maybe_unused]] const char* title
)
{
    m_sfTitle = title;
}

void
Client::Bar::appid(
    [[maybe_unused]] zdwl_ipc_output_v2* zdwl_ipc_output_v2,
    [[maybe_unused]] const char* appid
)
{
    m_sfAppid = appid;
}

void
Client::Bar::layoutSymbol(
    [[maybe_unused]] zdwl_ipc_output_v2* zdwl_ipc_output_v2,
    [[maybe_unused]] const char* layout
)
{
    m_sfLayoutIcon = layout;
}

void
Client::Bar::keyboardLayout(
    [[maybe_unused]] zdwl_ipc_output_v2* zdwl_ipc_output_v2,
    [[maybe_unused]] const char* kblayout
)
{
    m_sfKbLayout = kblayout;
}

void
Client::Bar::frame([[maybe_unused]] zdwl_ipc_output_v2* zdwl_ipc_output_v2)
{
    frame::g_bRedraw = true;
}

void
Client::Bar::fullscreen(
    [[maybe_unused]] zdwl_ipc_output_v2* zdwl_ipc_output_v2,
    [[maybe_unused]] u32 is_fullscreen
)
{
}

void
Client::Bar::floating(
    [[maybe_unused]] zdwl_ipc_output_v2* zdwl_ipc_output_v2,
    [[maybe_unused]] u32 is_floating
)
{
}

} /* namespace wl */
