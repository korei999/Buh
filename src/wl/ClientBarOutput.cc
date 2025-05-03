#include "Client.hh"

#include "frame.hh"

#include "adt/StdAllocator.hh"
#include "adt/logs.hh"

using namespace adt;

namespace wl
{

void
Client::BarOutput::createBuffer(int width, int height)
{
    m_width = width;
    m_height = height;

    const int stride = m_stride * 4;
    const int shmPoolSize = m_height * stride;

    // wl_shm_create_pool();

    // int fd = shm::allocFile(shmPoolSize);
    // m_pClient->m_pPoolData = static_cast<u8*>(mmap(nullptr, shmPoolSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    // m_poolSize = shmPoolSize;
    // ADT_ASSERT_ALWAYS(m_pPoolData, "mmap() failed");
    //
    // m_pShmPool = wl_shm_create_pool(m_pShm, fd, shmPoolSize);
    // ADT_ASSERT_ALWAYS(m_pShmPool, "wl_shm_create_pool() failed");
    //
    // m_pBuffer = wl_shm_pool_create_buffer(m_pShmPool, 0, m_width, m_height, stride, WL_SHM_FORMAT_ARGB8888);
    // ADT_ASSERT_ALWAYS(m_pBuffer, "wl_shm_pool_create_buffer() failed");
    //
    // m_vTempBuff.setSize(m_pAlloc, surfaceBuffer().getStride());
    // m_vDepthBuffer.setSize(m_pAlloc, m_stride * m_height);
    // m_pSurfaceBufferBind = m_pPoolData;
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
