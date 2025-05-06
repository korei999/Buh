#include "Client.hh"

#include "frame.hh"

#include "adt/logs.hh"

using namespace adt;

namespace wayland
{

void
Client::pointerEnter(
    [[maybe_unused]] wl_pointer* wl_pointer,
    [[maybe_unused]] u32 serial,
    [[maybe_unused]] wl_surface* surface,
    [[maybe_unused]] wl_fixed_t surface_x,
    [[maybe_unused]] wl_fixed_t surface_y
)
{
    m_pointer.pLastEnterSufrace = surface;
}

void
Client::pointerLeave(
    [[maybe_unused]] wl_pointer* wl_pointer,
    [[maybe_unused]] u32 serial,
    [[maybe_unused]] wl_surface* surface
)
{
    m_pointer.pLastEnterSufrace = nullptr;
}

void
Client::pointerMotion(
    [[maybe_unused]] wl_pointer* wl_pointer,
    [[maybe_unused]] u32 time,
    [[maybe_unused]] wl_fixed_t surface_x,
    [[maybe_unused]] wl_fixed_t surface_y
)
{
    m_pointer.surfacePointerX = wl_fixed_to_double(surface_x);
    m_pointer.surfacePointerY = wl_fixed_to_double(surface_y);
}

void
Client::pointerButton(
    [[maybe_unused]] wl_pointer* wl_pointer,
    [[maybe_unused]] u32 serial,
    [[maybe_unused]] u32 time,
    [[maybe_unused]] u32 button,
    [[maybe_unused]] u32 state
)
{
    m_pointer.time = time;
    m_pointer.eButton = Client::Pointer::BUTTON(button);
    m_pointer.state = state;

    if (m_pointer.state) frame::g_bRedraw = true;
}

void
Client::pointerAxis(
    [[maybe_unused]] wl_pointer* wl_pointer,
    [[maybe_unused]] u32 time,
    [[maybe_unused]] u32 axis,
    [[maybe_unused]] wl_fixed_t value
)
{
}

void
Client::pointerFrame([[maybe_unused]] wl_pointer *wl_pointer)
{
}

} /* namespace wayland */
