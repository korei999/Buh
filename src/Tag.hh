#pragma once

#include "wl/wayland-protocols/dwl-ipc.h"

struct Tag
{
    zdwl_ipc_output_v2_tag_state eState {};
    int nClients {};
    bool bFocused {};
};
