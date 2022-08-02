#include "config.h"
#include "wtf/WaylandDisplay.h"

#if USE(GLIB)

namespace WTF {

struct wl_display* WaylandDisplay::singleton()
{
    static LazyNeverDestroyed<WaylandDisplay> waylandDisplay;
    static std::once_flag onceKey;

    std::call_once(onceKey, [&] {
        waylandDisplay.construct();
    });

    return waylandDisplay.get().m_wl_display;
}

WaylandDisplay::WaylandDisplay()
{
    static GSourceFuncs s_sourceFuncs;

    m_wl_display = wl_display_connect(nullptr);

    m_isReading = false;

    s_sourceFuncs.prepare = static_prepare;
    s_sourceFuncs.check = static_check;
    s_sourceFuncs.dispatch = static_dispatch;

    m_eventSource = reinterpret_cast<EventSource*>(g_source_new(&s_sourceFuncs, sizeof(EventSource)));
    m_eventSource->m_waylandDisplay = this;

    m_pfd.fd = wl_display_get_fd(m_wl_display);
    m_pfd.events = G_IO_IN | G_IO_ERR;
    g_source_add_poll(&m_eventSource->m_source, &m_pfd);

    g_source_set_can_recurse(&m_eventSource->m_source, TRUE);

    g_source_attach(&m_eventSource->m_source, RunLoop::main().mainContext());
}

gboolean WaylandDisplay::static_prepare(GSource* base, gint* timeout)
{
    auto* eventSource = reinterpret_cast<EventSource*>(base);
    return eventSource->m_waylandDisplay->prepare(timeout);
}

gboolean WaylandDisplay::static_check(GSource* base)
{
    auto* eventSource = reinterpret_cast<EventSource*>(base);
    return eventSource->m_waylandDisplay->check();
}

gboolean WaylandDisplay::static_dispatch(GSource* base, GSourceFunc callback, gpointer user_data)
{
    auto* eventSource = reinterpret_cast<EventSource*>(base);
    return eventSource->m_waylandDisplay->dispatch(callback, user_data);
}

gboolean WaylandDisplay::prepare(gint* timeout)
{
    *timeout = -1;

    if (m_isReading)
        return FALSE;
    m_isReading = true;

    while (wl_display_prepare_read(m_wl_display) != 0)
        wl_display_dispatch_pending(m_wl_display);

    wl_display_flush(m_wl_display);

    return FALSE;
}

gboolean WaylandDisplay::check()
{
    gboolean has_event;

    has_event = m_pfd.revents & G_IO_IN;

    if (!m_isReading)
        return has_event;

    m_isReading = false;

    if (!has_event)
        wl_display_cancel_read(m_wl_display);
    else
        wl_display_read_events(m_wl_display);

    return has_event;
}

gboolean WaylandDisplay::dispatch(GSourceFunc, gpointer)
{
    if (wl_display_dispatch_pending(m_wl_display) < 0)
        return FALSE;

    return TRUE;
}

} // namespace WTF

#endif // USE(GLIB)
