#pragma once

#if ENABLE(ENCRYPTED_MEDIA) && ENABLE(FBXCDM)

#include "CDMFbxcdm.h"
#include "CDMInstanceSession.h"
#include "CDMProxy.h"
#include "GStreamerEMEUtilities.h"
#include "MediaPlayerPrivate.h"
#include "SharedBuffer.h"
#include <wtf/Condition.h>
#include <wtf/VectorHash.h>

namespace WebCore {

class CDMProxyFactoryFbxcdm final : public CDMProxyFactory {
    WTF_MAKE_FAST_ALLOCATED;

public:
    static CDMProxyFactoryFbxcdm& singleton();

    virtual ~CDMProxyFactoryFbxcdm() = default;

    const Vector<String>& supportedKeySystems() const;

private:
    friend class NeverDestroyed<CDMProxyFactoryFbxcdm>;

    CDMProxyFactoryFbxcdm() = default;

    RefPtr<CDMProxy> createCDMProxy(const String&) final;
    bool supportsKeySystem(const String&) final;
};

class CDMProxyFbxcdm final : public CDMProxy, public CanMakeWeakPtr<CDMProxyFbxcdm, WeakPtrFactoryInitialization::Eager> {
public:
    CDMProxyFbxcdm(const String& keySystem);

    virtual ~CDMProxyFbxcdm() = default;

    const String& keySystem();
    int getKeysID(const KeyIDType& kid, WeakPtr<CDMProxyDecryptionClient>&& client);

private:
    String m_keySystem;
};

} // namespace WebCore

#endif // ENABLE(ENCRYPTED_MEDIA) && ENABLE(FBXCDM)
