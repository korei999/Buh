#include "PipeWire.hh"

#include "adt/logs.hh"
#include "adt/StdAllocator.hh"

using namespace adt;

#if defined __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#if defined __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

static const pw_core_events s_coreEvents {
    .version = PW_VERSION_CORE_EVENTS,
    .info = decltype(pw_core_events::info)(methodPointer(&PipeWire::coreEventsInfo)),
    .done = decltype(pw_core_events::done)(methodPointer(&PipeWire::coreEventsDone)),
    .error = decltype(pw_core_events::error)(methodPointer(&PipeWire::coreEventsError)),
};

static const pw_registry_events s_registryEvents {
    .version = PW_VERSION_REGISTRY_EVENTS,
    .global = decltype(pw_registry_events::global)(methodPointer(&PipeWire::registryEventsGlobal)),
};

static const pw_device_events s_deviceEvents {
    .version = PW_VERSION_DEVICE_EVENTS,
    .info = decltype(pw_device_events::info)(methodPointer(&PipeWire::Device::info)),
    .param = decltype(pw_device_events::param)(methodPointer(&PipeWire::Device::param)),
};

#if defined __clang__
    #pragma clang diagnostic pop
#endif

#if defined __GNUC__
    #pragma GCC diagnostic pop
#endif

PipeWire::PipeWire(InitFlag)
{
    LOG_NOTIFY("PipeWire INIT...\n");

    pw_init({}, {});

    ADT_ASSERT_ALWAYS(m_pMainLoop = pw_main_loop_new({}), "");
    ADT_ASSERT_ALWAYS(m_pCtx = pw_context_new(pw_main_loop_get_loop(m_pMainLoop), {}, {}), "");
    ADT_ASSERT_ALWAYS(m_pCore = pw_context_connect(m_pCtx, {}, {}), "");
    ADT_ASSERT_ALWAYS(m_pRegistry = pw_core_get_registry(m_pCore, PW_VERSION_REGISTRY, {}), "");

    pw_core_add_listener(m_pCore, &m_coreListener, &s_coreEvents, this);
    pw_registry_add_listener(m_pRegistry, &m_registry_listener, &s_registryEvents, this);

    m_sync = pw_core_sync(m_pCore, PW_ID_CORE, m_sync);
}

void
PipeWire::destroy()
{
}

void
PipeWire::registryEventsGlobal(
    uint32_t id,
    uint32_t permissions,
    const char* type,
    uint32_t version,
    const spa_dict* props
)
{
    LOG_GOOD("id: {}, permissions: {}, type: {}, version: {}\n\n",
        id, permissions, type, version
    );

    StringView svType = type;

    if (svType == PW_TYPE_INTERFACE_Device)
    {
        void* pProxy {};
        ADT_ASSERT_ALWAYS(pProxy = pw_registry_bind(m_pRegistry, id, type, PW_VERSION_DEVICE, 0), "");

        ListNode<Device>* pNode = m_lDevices.pushBack(StdAllocator::inst(), {
            .m_pPipeWire = this,
            .m_id = id,
            .m_pProxy = pProxy,
        });

        pw_device_add_listener(
            reinterpret_cast<pw_device*>(pProxy),
            &pNode->data.m_listener,
            &s_deviceEvents,
            &pNode->data
        );

    }
}

void
PipeWire::coreEventsInfo(const pw_core_info* info)
{
    LOG_GOOD("info: \n"
        "id: {}\n"
        "cookie: {}\n"
        "user_name: {}\n"
        "host_name: {}\n"
        "version: {}\n"
        "name: {}\n"
        ,
        info->id, info->cookie,
        info->user_name, info->host_name,
        info->version, info->name
    );

    const spa_dict_item* pItem {};
    spa_dict_for_each(pItem, info->props)
        LOG("key/val: [{}, {}]\n", pItem->key, pItem->value);
}

void
PipeWire::coreEventsDone(uint32_t id, int seq)
{
    LOG_BAD("done: id: {}, seq: {}\n", id, seq);
}

void
PipeWire::coreEventsError(uint32_t id, int seq, int res, const char* message)
{
}

void
PipeWire::Device::info(const pw_device_info* info)
{
    if (!(info->change_mask & PW_DEVICE_CHANGE_MASK_PARAMS))
        return;

    for (u32 i = 0; i < info->n_params; ++i)
    {
        if (info->params[i].id == SPA_PARAM_Route)
        {
            pw_device_enum_params(
                reinterpret_cast<pw_device*>(m_pProxy),
                0, info->params[i].id, 0, -1, NULL
            );

            break;
        }
    }
}

void
PipeWire::Device::param(int seq, uint32_t id, uint32_t index, uint32_t next, const spa_pod* param)
{
    ADT_ASSERT(spa_pod_is_object_type(param, SPA_TYPE_OBJECT_ParamRoute), "");


}
