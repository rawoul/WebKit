#include "config.h"
#include "wtf/Fbxbus.h"

#if USE(GLIB) && USE(FBX_API)

extern "C" {
#include <libfbxbus.h>
#include <libfbxevent-gsource.h>
#include <libfbxevent.h>
}

namespace WTF {

struct fbxbus_ctx* Fbxbus::singleton()
{
    static LazyNeverDestroyed<Fbxbus> fbxbus;
    static std::once_flag onceKey;

    ASSERT(isMainThread());

    std::call_once(onceKey, [&] {
        fbxbus.construct();
    });

    return fbxbus.get().m_bus_ctx;
}

Fbxbus::Fbxbus()
{
    ASSERT(isMainThread());

    m_ev_ctx = fbxevent_init();
    ASSERT(m_ev_ctx);

    m_bus_ctx = fbxbus_create(m_ev_ctx);
    ASSERT(m_bus_ctx);

    fbxbus_connect(m_bus_ctx);

    m_source = fbxevent_new_gsource(m_ev_ctx);

    g_source_attach(m_source, RunLoop::main().mainContext());
}

} // namespace WTF

#endif // USE(GLIB) && USE(FBX_API)
