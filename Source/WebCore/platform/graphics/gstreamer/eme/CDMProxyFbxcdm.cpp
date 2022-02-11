#include "config.h"
#include "CDMProxyFbxcdm.h"

#if ENABLE(ENCRYPTED_MEDIA) && ENABLE(FBXCDM)

#include "CDMFbxcdm.h"

namespace WebCore {

// CDMProxyFactoryFbxcdm ======================================================

CDMProxyFactoryFbxcdm& CDMProxyFactoryFbxcdm::singleton()
{
    static NeverDestroyed<CDMProxyFactoryFbxcdm> s_factory;
    return s_factory;
}

RefPtr<CDMProxy> CDMProxyFactoryFbxcdm::createCDMProxy(const String& keySystem)
{
    return adoptRef(new CDMProxyFbxcdm(keySystem));
}

bool CDMProxyFactoryFbxcdm::supportsKeySystem(const String& keySystem)
{
    return supportedKeySystems().contains(keySystem);
}

const Vector<String>& CDMProxyFactoryFbxcdm::supportedKeySystems() const
{
    return CDMFactoryFbxcdm::singleton().supportedKeySystems();
}

// CDMProxyFbxcdm =============================================================

CDMProxyFbxcdm::CDMProxyFbxcdm(const String& keySystem)
    : m_keySystem(keySystem)
{
}

const String& CDMProxyFbxcdm::keySystem()
{
    return m_keySystem;
}

int CDMProxyFbxcdm::getKeysID(const KeyIDType& kid, WeakPtr<CDMProxyDecryptionClient>&& client)
{
    auto keyHandleOpt = getOrWaitForKeyHandle(kid, WTFMove(client));
    if (!keyHandleOpt.has_value())
        return -1;

    const auto& keyHandleRef = keyHandleOpt.value();
    if (!keyHandleRef->isStatusCurrentlyValid())
        return -1;

    const CDMInstanceProxy* instanceProxy = instance();
    if (!instanceProxy)
        return -1;

    const CDMInstanceProxyFbxcdm* instanceProxyFbxcdm = static_cast<const CDMInstanceProxyFbxcdm*>(instanceProxy);
    auto& keys = instanceProxyFbxcdm->fbxcdmKeys();
    int keys_id = keys.id();
    if (keys_id < 0)
        return -1;

    return keys_id;
}

} // namespace WebCore

#endif // ENABLE(ENCRYPTED_MEDIA) && ENABLE(FBXCDM)
