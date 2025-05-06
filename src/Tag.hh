#pragma once

#include "wl/WaylandGlueGenerated/dwl-ipc.h"

struct Tag
{
    zdwl_ipc_output_v2_tag_state eState {};
    int nClients {};
    bool bFocused {};
};
