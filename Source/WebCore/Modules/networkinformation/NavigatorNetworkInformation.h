#pragma once

#include "DOMWindowProperty.h"
#include "Supplementable.h"
#include <wtf/Forward.h>

namespace WebCore {

class Navigator;
class NetworkInformation;

class NavigatorNetworkInformation final : public Supplement<Navigator>, public DOMWindowProperty {
    WTF_MAKE_FAST_ALLOCATED;
public:
    explicit NavigatorNetworkInformation(DOMWindow*);
    ~NavigatorNetworkInformation();

    static RefPtr<NetworkInformation> connection(Navigator&);
    RefPtr<NetworkInformation> connection();

private:
    static NavigatorNetworkInformation* from(Navigator&);
    static const char* supplementName();

    RefPtr<NetworkInformation> m_connection;
};

}
