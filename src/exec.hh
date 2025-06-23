#pragma once

#if __has_include("configUser.hh") && defined OPT_USER_CONFIG
    #include "configUser.hh"
#else
    #include "configDefault.hh"
#endif /* __has_include("configUser.hh") && defined OPT_USER_CONFIG */

namespace exec
{

config::String64 runReadOutput(const config::StatusEntry& entry);

} /* namespace exec */
