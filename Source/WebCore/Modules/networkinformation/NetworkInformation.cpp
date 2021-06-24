#include "config.h"

#include "NetworkInformation.h"
#include "Document.h"
#include "Event.h"
#include "EventNames.h"
#include <wtf/IsoMallocInlines.h>

#if USE(FBX_API)
# include <wtf/Fbxbus.h>
# include <fbxconnman.h>
#endif

namespace WebCore {

WTF_MAKE_ISO_ALLOCATED_IMPL(NetworkInformation);

NetworkInformation::NetworkInformation(Document& document)
    : ActiveDOMObject(document)
{
#if USE(FBX_API)
    status_changed = fbxconnman_bind_status_change(Fbxbus::singleton(),
            fbxconnman_cb_status_change, this);
#endif

    updateNetworkInformation();
}

NetworkInformation::~NetworkInformation()
{
#if USE(FBX_API)
    if (status_changed)
        fbxconnman_unbind(Fbxbus::singleton(), status_changed);
#endif
}

Ref<NetworkInformation> NetworkInformation::create(Document& document)
{
    auto connection = adoptRef(*new NetworkInformation(document));
    connection->suspendIfNeeded();
    return connection;
}

bool NetworkInformation::updateNetworkInformation()
{
    bool changed = false;
#if USE(FBX_API)
    struct fbxconnman_status status;
    struct fbxconnman_connection conn;
    int ret;

    ret = fbxconnman_status_get(Fbxbus::singleton(), &status);
    if (ret != FBXCONNMAN_SUCCESS)
        return false;

    ConnectionType type = ConnectionType::None;
    if (*status.connection) {
        ret = fbxconnman_conn_get(Fbxbus::singleton(), status.connection, &conn);
        if (ret != FBXCONNMAN_SUCCESS)
            return false;
        if (conn.type == FBXCONNMAN_CONN_TYPE_ETHERNET)
            type = ConnectionType::Ethernet;
        else if (conn.type == FBXCONNMAN_CONN_TYPE_WIFI)
            type = ConnectionType::Wifi;
        else
            type = ConnectionType::Unknown;
    }

    if (m_type != type) {
        m_type = type;
        changed = true;
    }
#endif

    return changed;
}

void NetworkInformation::sendchangeEvent()
{
    queueTaskToDispatchEvent(*this, TaskSource::Networking, Event::create(eventNames().changeEvent, Event::CanBubble::No, Event::IsCancelable::No));
}

#if USE(FBX_API)
void NetworkInformation::fbxconnman_cb_status_change(void* priv)
{
    auto& netinfo = *reinterpret_cast<NetworkInformation*>(priv);
    if (netinfo.updateNetworkInformation())
        netinfo.sendchangeEvent();
}
#endif

}
