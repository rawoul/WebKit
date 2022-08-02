#pragma once

#if USE(GLIB)

#include <wayland-client.h>

#include <wtf/RunLoop.h>

namespace WTF {

class WaylandDisplay {
public:
    static struct wl_display* singleton();

private:
    struct EventSource {
        GSource m_source;
        WaylandDisplay* m_waylandDisplay;
    };

    static gboolean static_prepare(GSource*, gint*);
    static gboolean static_check(GSource*);
    static gboolean static_dispatch(GSource*, GSourceFunc, gpointer);

    struct wl_display* m_wl_display;
    bool m_isReading;
    EventSource* m_eventSource;
    GPollFD m_pfd;

    WaylandDisplay();
    gboolean prepare(gint*);
    gboolean check();
    gboolean dispatch(GSourceFunc, gpointer);

    WaylandDisplay(const WaylandDisplay&) = delete;
    void operator=(const WaylandDisplay&) = delete;
    WaylandDisplay(WaylandDisplay&&) = delete;
    WaylandDisplay& operator=(WaylandDisplay&&) = delete;
    ~WaylandDisplay() = delete;

    friend class LazyNeverDestroyed<WaylandDisplay>;
};

} // namespace WTF

using WTF::WaylandDisplay;

#endif // USE(GLIB)
