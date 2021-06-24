#pragma once

#include "ActiveDOMObject.h"
#include "ConnectionType.h"
#include "EventTarget.h"

struct fbxconnman_signal;

namespace WebCore {

class NetworkInformation : public RefCounted<NetworkInformation>
    , public ActiveDOMObject
    , public EventTargetWithInlineData {
    WTF_MAKE_ISO_ALLOCATED(NetworkInformation);
public:
    static Ref<NetworkInformation> create(Document&);
    virtual ~NetworkInformation();

    ConnectionType type() const { return m_type; }
    double downlinkMax() const { return m_downlinkMaxMbps; }

    using RefCounted::ref;
    using RefCounted::deref;

private:
    explicit NetworkInformation(Document&);

    // EventTarget
    void refEventTarget() final { ref(); }
    void derefEventTarget() final { deref(); }

    // ActiveDOMObject.
    const char* activeDOMObjectName() const final { return "NetworkInformation"; }

    // EventTargetWithInlineData.
    EventTargetInterface eventTargetInterface() const final { return NetworkInformationEventTargetInterfaceType; }
    ScriptExecutionContext* scriptExecutionContext() const final { return ActiveDOMObject::scriptExecutionContext(); }

    bool updateNetworkInformation();
    void sendchangeEvent();

    double m_downlinkMaxMbps = 0;
    ConnectionType m_type { ConnectionType::Unknown };

#if USE(FBX_API)
    struct fbxconnman_signal *status_changed;
    static void fbxconnman_cb_status_change(void* priv);
#endif // USE(FBX_API)
};

}
