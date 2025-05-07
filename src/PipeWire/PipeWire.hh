#pragma once

#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>

#include "adt/List.hh"

struct PipeWire
{
    struct Device
    {
        PipeWire* m_pPipeWire {};
        adt::u32 m_id {};
        void* m_pProxy {};
        struct spa_hook m_listener {};

        /* */

        void info(const pw_device_info* info);
        void param(int seq, uint32_t id, uint32_t index, uint32_t next, const spa_pod* param);
    };

    /* */

    pw_main_loop* m_pMainLoop {};
    pw_context* m_pCtx {};
    pw_core* m_pCore {};
    pw_registry* m_pRegistry {};

    spa_hook m_registry_listener {};
    spa_hook m_coreListener {};

    int m_sync {};

    adt::List<Device> m_lDevices {};

    /* */

    PipeWire() = default;
    PipeWire(adt::InitFlag);

    /* */

    void destroy();
    void registryEventsGlobal(uint32_t id, uint32_t permissions, const char* type, uint32_t version, const spa_dict* props);

    void coreEventsInfo(const pw_core_info* info);
    void coreEventsDone(uint32_t id, int seq);
    void coreEventsError(uint32_t id, int seq, int res, const char* message);
};
