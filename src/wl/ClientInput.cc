#include "Client.hh"

#include "frame.hh"

#include "adt/logs.hh"

namespace wl
{

void
Client::pointerEnter(
    wl_pointer* wl_pointer,
    uint32_t serial,
    wl_surface* surface,
    wl_fixed_t surface_x,
    wl_fixed_t surface_y
)
{
    m_pointer.pLastEnterSufrace = surface;
}

void
Client::pointerLeave(
    wl_pointer* wl_pointer,
    uint32_t serial,
    wl_surface* surface
)
{
    m_pointer.pLastEnterSufrace = nullptr;
}

void
Client::pointerMotion(
    wl_pointer* wl_pointer,
    uint32_t time,
    wl_fixed_t surface_x,
    wl_fixed_t surface_y
)
{
    m_pointer.surfacePointerX = wl_fixed_to_double(surface_x);
    m_pointer.surfacePointerY = wl_fixed_to_double(surface_y);
    // LOG_NOTIFY("xy: [{}, {}]\n", wl_fixed_to_double(surface_x), wl_fixed_to_double(surface_y));
}

void
Client::pointerButton(
    wl_pointer* wl_pointer,
    uint32_t serial,
    uint32_t time,
    uint32_t button,
    uint32_t state
)
{
    m_pointer.time = time;
    m_pointer.eButton = Client::Pointer::BUTTON(button);
    m_pointer.state = state;

    // LOG_NOTIFY("button: {}, state: {}\n", button, state);

    if (m_pointer.state) frame::g_bRedraw = true;
}

void
Client::pointerAxis(
    wl_pointer* wl_pointer,
    uint32_t time,
    uint32_t axis,
    wl_fixed_t value
)
{
}

void
Client::pointerFrame(wl_pointer *wl_pointer)
{
}

void
Client::dwlTags(
    zdwl_ipc_manager_v2* zdwl_ipc_manager_v2,
    uint32_t amount
)
{
    LOG_BAD("({}), amount: '{}'\n", zdwl_ipc_manager_v2, amount);
}

void
Client::dwlLayout(
    zdwl_ipc_manager_v2* zdwl_ipc_manager_v2,
    const char* name
)
{
    LOG_BAD("({}), name: '{}'\n", zdwl_ipc_manager_v2, name);
}

} /* namespace wl */
