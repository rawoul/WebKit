#pragma once

#if USE(GLIB) && USE(FBX_API)

#include <wtf/RunLoop.h>

struct fbxevent_ctx;
struct fbxbus_ctx;

namespace WTF {

class Fbxbus {
public:
    static struct fbxbus_ctx* singleton();

private:
    struct fbxevent_ctx* m_ev_ctx;
    struct fbxbus_ctx* m_bus_ctx;
    GSource* m_source;

    Fbxbus();

    Fbxbus(const Fbxbus&) = delete;
    void operator=(const Fbxbus&) = delete;
    Fbxbus(Fbxbus&&) = delete;
    Fbxbus& operator=(Fbxbus&&) = delete;
    ~Fbxbus() = delete;

    friend class LazyNeverDestroyed<Fbxbus>;
};

} // namespace WTF

using WTF::Fbxbus;

#endif // USE(GLIB) && USE(FBX_API)
