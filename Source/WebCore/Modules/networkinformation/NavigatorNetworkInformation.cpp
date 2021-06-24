#include "config.h"
#include "NavigatorNetworkInformation.h"

#include "Frame.h"
#include "Navigator.h"
#include "NetworkInformation.h"

namespace WebCore {

NavigatorNetworkInformation::NavigatorNetworkInformation(DOMWindow* window)
    : DOMWindowProperty(window)
{
}

NavigatorNetworkInformation::~NavigatorNetworkInformation() = default;

RefPtr<NetworkInformation> NavigatorNetworkInformation::connection(Navigator& navigator)
{
    return NavigatorNetworkInformation::from(navigator)->connection();
}

RefPtr<NetworkInformation> NavigatorNetworkInformation::connection()
{
    if (!m_connection && frame())
        m_connection = NetworkInformation::create(*frame()->document());
    return m_connection;
}

NavigatorNetworkInformation* NavigatorNetworkInformation::from(Navigator& navigator)
{
    auto* supplement = static_cast<NavigatorNetworkInformation*>(Supplement<Navigator>::from(&navigator, supplementName()));
    if (!supplement) {
        auto newSupplement = makeUnique<NavigatorNetworkInformation>(navigator.window());
        supplement = newSupplement.get();
        provideTo(&navigator, supplementName(), WTFMove(newSupplement));
    }
    return supplement;
}

const char* NavigatorNetworkInformation::supplementName()
{
    return "NavigatorNetworkInformation";
}

} // namespace WebCore
