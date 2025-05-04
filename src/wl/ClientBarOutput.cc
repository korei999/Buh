#include "Client.hh"

#include "wl/shm.hh"
#include "frame.hh"

#include "adt/StdAllocator.hh"
#include "adt/defer.hh"

using namespace adt;

namespace wl
{

void
Client::BarOutput::allocShmBuffer()
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
Client::BarOutput::destroy()
{
    wl_shm_pool_destroy(m_pShmPool);
    munmap(m_pPoolData, m_poolSize);
    wl_output_destroy(m_pOutput);
    zwlr_layer_surface_v1_destroy(m_pLayerSurface);
    wl_surface_destroy(m_pSurface);
    zdwl_ipc_output_v2_destroy(m_pDwlOutput);
    wl_buffer_destroy(m_pBuffer);
}

void
Client::BarOutput::toggleVisibility(zdwl_ipc_output_v2* zdwl_ipc_output_v2)
{
}

void
Client::BarOutput::active(zdwl_ipc_output_v2* zdwl_ipc_output_v2, uint32_t active)
{
}

void
Client::BarOutput::tag(
    zdwl_ipc_output_v2* zdwl_ipc_output_v2,
    uint32_t tag,
    uint32_t state,
    uint32_t clients,
    uint32_t focused
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
Client::BarOutput::layout(zdwl_ipc_output_v2* zdwl_ipc_output_v2, uint32_t layout)
{
}

void
Client::BarOutput::title(zdwl_ipc_output_v2* zdwl_ipc_output_v2, const char* title)
{
    m_sfTitle = title;
}

void
Client::BarOutput::appid(zdwl_ipc_output_v2* zdwl_ipc_output_v2, const char* appid)
{
    m_sfAppid = appid;
}

void
Client::BarOutput::layoutSymbol(zdwl_ipc_output_v2* zdwl_ipc_output_v2, const char* layout)
{
    m_sfLayoutIcon = layout;
}

void
Client::BarOutput::keyboardLayout(zdwl_ipc_output_v2* zdwl_ipc_output_v2, const char* kblayout)
{
    m_sfKbLayout = kblayout;
}

void
Client::BarOutput::frame(zdwl_ipc_output_v2* zdwl_ipc_output_v2)
{
    frame::g_bRedraw = true;
}

void
Client::BarOutput::fullscreen(zdwl_ipc_output_v2* zdwl_ipc_output_v2, uint32_t is_fullscreen)
{
}

void
Client::BarOutput::floating(zdwl_ipc_output_v2* zdwl_ipc_output_v2, uint32_t is_floating)
{
}

} /* namespace wl */
