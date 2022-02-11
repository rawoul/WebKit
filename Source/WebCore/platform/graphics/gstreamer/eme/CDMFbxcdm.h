#pragma once

#if ENABLE(ENCRYPTED_MEDIA) && ENABLE(FBXCDM)

#include "CDMFactory.h"
#include "CDMInstanceSession.h"
#include "CDMPrivate.h"
#include "CDMProxy.h"
#include "FbxcdmClientxx.h"
#include "MediaKeyStatus.h"
#include "SharedBuffer.h"
#include <wtf/WeakPtr.h>

namespace WebCore {

class CDMFactoryFbxcdm final : public CDMFactory {
    WTF_MAKE_FAST_ALLOCATED;

public:
    virtual ~CDMFactoryFbxcdm() = default;

    static CDMFactoryFbxcdm& singleton();

    std::unique_ptr<CDMPrivate> createCDM(const String&, const CDMPrivateClient&) final;
    bool supportsKeySystem(const String&) final;

    const Vector<String>& supportedKeySystems() const;

private:
    friend class NeverDestroyed<CDMFactoryFbxcdm>;
    CDMFactoryFbxcdm();
};

class CDMPrivateFbxcdm final : public CDMPrivate {
    WTF_MAKE_FAST_ALLOCATED;

public:
    virtual ~CDMPrivateFbxcdm() = default;

    CDMPrivateFbxcdm(const String&);

    Vector<AtomString> supportedInitDataTypes() const final;
    Vector<AtomString> supportedRobustnesses() const final;
    bool supportsConfiguration(const CDMKeySystemConfiguration&) const final;
    bool supportsConfigurationWithRestrictions(const CDMKeySystemConfiguration&, const CDMRestrictions&) const final;
    bool supportsSessionTypeWithConfiguration(const CDMSessionType&, const CDMKeySystemConfiguration&) const final;
    CDMRequirement distinctiveIdentifiersRequirement(const CDMKeySystemConfiguration&, const CDMRestrictions&) const final;
    CDMRequirement persistentStateRequirement(const CDMKeySystemConfiguration&, const CDMRestrictions&) const final;
    bool distinctiveIdentifiersAreUniquePerOriginAndClearable(const CDMKeySystemConfiguration&) const final;
    RefPtr<CDMInstance> createInstance() final;
    void loadAndInitialize() final;
    bool supportsServerCertificates() const final;
    bool supportsSessions() const final;
    bool supportsInitData(const AtomString&, const SharedBuffer&) const final;
    RefPtr<SharedBuffer> sanitizeResponse(const SharedBuffer&) const final;
    std::optional<String> sanitizeSessionId(const String&) const final;

private:
    String m_keySystem;
};

class CDMInstanceProxyFbxcdm final : public CDMInstanceProxy {
public:
    virtual ~CDMInstanceProxyFbxcdm() = default;

    CDMInstanceProxyFbxcdm(const String&);

    ImplementationType implementationType() const final;
    void initializeWithConfiguration(const CDMKeySystemConfiguration&, AllowDistinctiveIdentifiers, AllowPersistentState, SuccessCallback&&) final;
    void setServerCertificate(Ref<SharedBuffer>&&, SuccessCallback&&) final;
    void setStorageDirectory(const String&) final;
    const String& keySystem() const final;
    RefPtr<CDMInstanceSession> createSession() final;

    Fbxcdm::MediaKeys& fbxcdmKeys() const;

private:
    String m_keySystem;
    RefPtr<Fbxcdm::MediaKeys> m_fbxcdmKeys;
};

class CDMInstanceSessionProxyFbxcdm final : public CDMInstanceSessionProxy {
public:
    CDMInstanceSessionProxyFbxcdm(CDMInstanceProxyFbxcdm&);

    void setClient(WeakPtr<CDMInstanceSessionClient>&&) final;
    void clearClient() final;
    void requestLicense(LicenseType, const AtomString&, Ref<SharedBuffer>&&, LicenseCallback&&) final;
    void updateLicense(const String&, LicenseType, Ref<SharedBuffer>&&, LicenseUpdateCallback&&) final;
    void loadSession(LicenseType, const String&, const String&, LoadSessionCallback&&) final;
    void closeSession(const String&, CloseSessionCallback&&) final;
    void removeSessionData(const String&, LicenseType, RemoveSessionDataCallback&&) final;
    void storeRecordOfKeyUsage(const String&) final;

    void challengeGeneratedCallback(CDMInstanceSession::MessageType, Ref<SharedBuffer>&&);
    void keyUpdatedCallback(KeyIDType&&, CDMInstanceSession::KeyStatus);
    void keysUpdateDoneCallback();

private:
    using Notification = void (CDMInstanceSessionProxyFbxcdm::*)(RefPtr<WebCore::SharedBuffer>&&);
    using ChallengeGeneratedCallback = Function<void(CDMInstanceSession::MessageType, Ref<SharedBuffer>&&)>;
    using SessionChangedCallback = Function<void(bool, RefPtr<SharedBuffer>&&)>;

    CDMInstanceProxyFbxcdm* cdmInstanceProxyFbxcdm() const;
    void sessionFailure();

    String m_sessionID;
    KeyStore m_keyStore;
    bool m_doesKeyStoreNeedMerging { false };
    RefPtr<Fbxcdm::MediaKeySession> m_session;
    Vector<ChallengeGeneratedCallback> m_challengeCallbacks;
    Vector<SessionChangedCallback> m_sessionChangedCallbacks;
    WeakPtr<CDMInstanceSessionClient> m_client;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_CDM_INSTANCE(WebCore::CDMInstanceProxyFbxcdm, WebCore::CDMInstance::ImplementationType::Fbxcdm);

#endif // ENABLE(ENCRYPTED_MEDIA) && ENABLE(FBXCDM)
