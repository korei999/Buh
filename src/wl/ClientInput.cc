#include "Client.hh"

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
}

void
Client::pointerLeave(
    wl_pointer* wl_pointer,
    uint32_t serial,
    wl_surface* surface
)
{
}

void
Client::pointerMotion(
    wl_pointer* wl_pointer,
    uint32_t time,
    wl_fixed_t surface_x,
    wl_fixed_t surface_y
)
{
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
Client::keyboardKeymap(
    wl_keyboard* wl_keyboard,
    uint32_t format,
    int32_t fd,
    uint32_t size
)
{
}

void
Client::keyboardEnter(
    wl_keyboard* wl_keyboard,
    uint32_t serial,
    wl_surface* surface,
    wl_array* keys
)
{
}

void
Client::keyboardLeave(
    wl_keyboard* wl_keyboard,
    uint32_t serial,
    wl_surface* surface
)
{
}

void
Client::keyboardKey(
    wl_keyboard* wl_keyboard,
    uint32_t serial,
    uint32_t time,
    uint32_t key,
    uint32_t state
)
{
}

void
Client::keyboardModifiers(
    wl_keyboard* wl_keyboard,
    uint32_t serial,
    uint32_t mods_depressed,
    uint32_t mods_latched,
    uint32_t mods_locked,
    uint32_t group
)
{
}

void
Client::keyboardRepeatInfo(
    wl_keyboard* wl_keyboard,
    int32_t rate,
    int32_t delay
)
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
