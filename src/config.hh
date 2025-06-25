#if  defined OPT_USER_CONFIG && __has_include("configUser.hh")
    #include "configUser.hh"
#else
    #include "configDefault.hh"
#endif /* __has_include("configUser.hh") && defined OPT_USER_CONFIG */
