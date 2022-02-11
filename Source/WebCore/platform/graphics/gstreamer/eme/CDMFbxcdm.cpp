#include "config.h"
#include "CDMFbxcdm.h"

#if ENABLE(ENCRYPTED_MEDIA) && ENABLE(FBXCDM)

#include "CDMKeySystemConfiguration.h"
#include "CDMProxyFbxcdm.h"
#include "CDMRestrictions.h"
#include "CDMSessionType.h"
#include "CDMUtilities.h"
#include "GStreamerEMEUtilities.h"
#include "InitDataRegistry.h"
#include "Logging.h"
#include "MediaKeyMessageType.h"
#include "NotImplemented.h"
#include "SharedBuffer.h"
#include <algorithm>
#include <iterator>
#include <wtf/MainThread.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/text/Base64.h>
#include <wtf/text/StringToIntegerConversion.h>

namespace WebCore {

static CDMInstanceSession::SessionLoadFailure sessionLoadFailureFromFbxcdm(const StringView& loadStatus)
{
    if (loadStatus == "None"_s)
        return CDMInstanceSession::SessionLoadFailure::None;

    if (loadStatus == "SessionNotFound"_s)
        return CDMInstanceSession::SessionLoadFailure::NoSessionData;

    if (loadStatus == "MismatchedSessionType"_s)
        return CDMInstanceSession::SessionLoadFailure::MismatchedSessionType;

    if (loadStatus == "QuotaExceeded"_s)
        return CDMInstanceSession::SessionLoadFailure::QuotaExceeded;

    return CDMInstanceSession::SessionLoadFailure::Other;
}

//=============================================================================
// CDMFactoryFbxcdm:

CDMFactoryFbxcdm& CDMFactoryFbxcdm::singleton()
{
    static NeverDestroyed<CDMFactoryFbxcdm> s_factory;
    return s_factory;
}

CDMFactoryFbxcdm::CDMFactoryFbxcdm()
    : CDMFactory()
{
}

std::unique_ptr<CDMPrivate> CDMFactoryFbxcdm::createCDM(const String& keySystem, const CDMPrivateClient&)
{
    return makeUnique<CDMPrivateFbxcdm>(keySystem);
}

bool CDMFactoryFbxcdm::supportsKeySystem(const String& keySystem)
{
    return supportedKeySystems().contains(keySystem);
}

const Vector<String>& CDMFactoryFbxcdm::supportedKeySystems() const
{
    static Vector<String> supportedKeySystems;

    if (supportedKeySystems.isEmpty()) {
        String s = String::fromLatin1(GStreamerEMEUtilities::s_WidevineKeySystem);
        auto mksaRef = Fbxcdm::Client::singleton().requestMediaKeySystemAccess(s);
        if (mksaRef)
            supportedKeySystems.append(s);
    }

    return supportedKeySystems;
}

//=============================================================================
// CDMPrivateFbxcdm:

CDMPrivateFbxcdm::CDMPrivateFbxcdm(const String& keySystem)
    : CDMPrivate()
    , m_keySystem(keySystem)
{
}

Vector<AtomString> CDMPrivateFbxcdm::supportedInitDataTypes() const
{
    return {
        InitDataRegistry::cencName(),
    };
}

Vector<AtomString> CDMPrivateFbxcdm::supportedRobustnesses() const
{
    return {
        emptyAtom(),
        "SW_SECURE_CRYPTO"_s,
        "SW_SECURE_DECODE"_s,
        "HW_SECURE_CRYPTO"_s,
        "HW_SECURE_DECODE"_s,
        "HW_SECURE_ALL"_s,
    };
}

bool CDMPrivateFbxcdm::supportsConfiguration(const CDMKeySystemConfiguration& configuration) const
{
    auto mksaRef = Fbxcdm::Client::singleton().requestMediaKeySystemAccess(m_keySystem);

    for (auto& audioCapability : configuration.audioCapabilities) {
        if (!mksaRef->supportsContentType(audioCapability.contentType))
            return false;
    }

    for (auto& videoCapability : configuration.videoCapabilities) {
        if (!mksaRef->supportsContentType(videoCapability.contentType))
            return false;
    }

    return true;
}

bool CDMPrivateFbxcdm::supportsConfigurationWithRestrictions(const CDMKeySystemConfiguration& configuration, const CDMRestrictions& restrictions) const
{
    UNUSED_PARAM(restrictions);
    return supportsConfiguration(configuration);
}

bool CDMPrivateFbxcdm::supportsSessionTypeWithConfiguration(const CDMSessionType& sessionType, const CDMKeySystemConfiguration& configuration) const
{
    UNUSED_PARAM(sessionType);
    return supportsConfiguration(configuration);
}

CDMRequirement CDMPrivateFbxcdm::distinctiveIdentifiersRequirement(const CDMKeySystemConfiguration& accumulatedConfiguration, const CDMRestrictions& restrictions) const
{
    UNUSED_PARAM(accumulatedConfiguration);
    UNUSED_PARAM(restrictions);
    return CDMRequirement::Optional;
}

CDMRequirement CDMPrivateFbxcdm::persistentStateRequirement(const CDMKeySystemConfiguration& accumulatedConfiguration, const CDMRestrictions& restrictions) const
{
    UNUSED_PARAM(accumulatedConfiguration);
    UNUSED_PARAM(restrictions);
    return CDMRequirement::Optional;
}

bool CDMPrivateFbxcdm::distinctiveIdentifiersAreUniquePerOriginAndClearable(const CDMKeySystemConfiguration& accumulatedConfiguration) const
{
    UNUSED_PARAM(accumulatedConfiguration);
    return false;
}

RefPtr<CDMInstance> CDMPrivateFbxcdm::createInstance()
{
    return adoptRef(new CDMInstanceProxyFbxcdm(m_keySystem));
}

void CDMPrivateFbxcdm::loadAndInitialize()
{
    // No-op.
}

bool CDMPrivateFbxcdm::supportsServerCertificates() const
{
    return true;
}

bool CDMPrivateFbxcdm::supportsSessions() const
{
    // Sessions are supported.
    return true;
}

bool CDMPrivateFbxcdm::supportsInitData(const AtomString& initDataType, const SharedBuffer& initData) const
{
    // Validate the initData buffer as CENC initData. FIXME: Validate it is actually CENC.
    if (equalLettersIgnoringASCIICase(initDataType, "cenc"_s) && !initData.isEmpty())
        return true;

    return false;
}

RefPtr<SharedBuffer> CDMPrivateFbxcdm::sanitizeResponse(const SharedBuffer& response) const
{
    return response.makeContiguous();
}

std::optional<String> CDMPrivateFbxcdm::sanitizeSessionId(const String& sessionId) const
{
    return sessionId;
}

//=============================================================================
// CDMInstanceProxyFbxcdm:

CDMInstanceProxyFbxcdm::CDMInstanceProxyFbxcdm(const String& keySystem)
    : CDMInstanceProxy(keySystem)
    , m_keySystem(keySystem)
    , m_fbxcdmKeys()
{
    auto mksaRef = Fbxcdm::Client::singleton().requestMediaKeySystemAccess(keySystem);
    m_fbxcdmKeys = mksaRef->createMediaKeys();
}

CDMInstance::ImplementationType CDMInstanceProxyFbxcdm::implementationType() const
{
    return CDMInstance::ImplementationType::Fbxcdm;
}

void CDMInstanceProxyFbxcdm::initializeWithConfiguration(const CDMKeySystemConfiguration& configuration, AllowDistinctiveIdentifiers allowDistinctiveIdentifiers, AllowPersistentState allowPersistentState, SuccessCallback&& callback)
{
    UNUSED_PARAM(configuration);
    UNUSED_PARAM(allowDistinctiveIdentifiers);
    UNUSED_PARAM(allowPersistentState);
    callback(Succeeded);
}

void CDMInstanceProxyFbxcdm::setServerCertificate(Ref<SharedBuffer>&& certificate, SuccessCallback&& callback)
{
    auto& keys = fbxcdmKeys();
    bool b = keys.setServerCertificate(WTFMove(certificate));
    callback(b ? Succeeded : Failed);
}

void CDMInstanceProxyFbxcdm::setStorageDirectory(const String& storageDirectory)
{
    auto& keys = fbxcdmKeys();
    keys.setOrigin(storageDirectory);
}

const String& CDMInstanceProxyFbxcdm::keySystem() const
{
    return m_keySystem;
}

RefPtr<CDMInstanceSession> CDMInstanceProxyFbxcdm::createSession()
{
    return adoptRef(new CDMInstanceSessionProxyFbxcdm(*this));
}

Fbxcdm::MediaKeys& CDMInstanceProxyFbxcdm::fbxcdmKeys() const
{
    return *m_fbxcdmKeys.get();
}

//=============================================================================
// ParsedResponseMessage:

class ParsedResponseMessage {
public:
    ParsedResponseMessage(const RefPtr<SharedBuffer>& buffer)
    {
        if (!buffer || !buffer->size())
            return;

        StringView payload(reinterpret_cast<const LChar*>(buffer->data()), buffer->size());
        static NeverDestroyed<StringView> type(reinterpret_cast<const LChar*>(":Type:"), 6);
        size_t typePosition = payload.find(type, 0);
        StringView requestType(payload.characters8(), (typePosition != notFound) ? typePosition : 0);
        unsigned offset = 0u;
        if (!requestType.isEmpty() && (requestType.length() != payload.length()))
            offset = typePosition + 6;

        if (requestType.length() == 1) {
            // FIXME: There are simpler ways to convert a single digit to a number than calling parseInteger.
            m_type = std::make_optional(static_cast<WebCore::MediaKeyMessageType>(parseInteger<int>(requestType).value_or(0)));
        }

        m_payload = SharedBuffer::create(payload.characters8() + offset, payload.length() - offset);

        m_isValid = true;
    }

    bool isValid() const
    {
        return m_isValid;
    }

    bool hasPayload() const
    {
        return static_cast<bool>(m_payload);
    }

    const Ref<SharedBuffer>& payload() const&
    {
        ASSERT(m_payload);
        return m_payload.value();
    }

    Ref<SharedBuffer>& payload() &
    {
        ASSERT(m_payload);
        return m_payload.value();
    }

    bool hasType() const
    {
        return m_type.has_value();
    }

    WebCore::MediaKeyMessageType type() const
    {
        ASSERT(m_type);
        return m_type.value();
    }

    WebCore::MediaKeyMessageType typeOr(WebCore::MediaKeyMessageType alternate) const
    {
        return m_type ? m_type.value() : alternate;
    }

    explicit operator bool() const
    {
        return m_isValid;
    }

    bool operator!() const
    {
        return !m_isValid;
    }

private:
    bool m_isValid { false };
    std::optional<Ref<SharedBuffer>> m_payload;
    std::optional<WebCore::MediaKeyMessageType> m_type;
};

//=============================================================================
// CDMInstanceSessionProxyFbxcdm:

CDMInstanceSessionProxyFbxcdm::CDMInstanceSessionProxyFbxcdm(CDMInstanceProxyFbxcdm& instance)
    : CDMInstanceSessionProxy(instance)
{
}

CDMInstanceProxyFbxcdm* CDMInstanceSessionProxyFbxcdm::cdmInstanceProxyFbxcdm() const
{
    auto proxy = cdmInstanceProxy();
    return static_cast<CDMInstanceProxyFbxcdm*>(proxy.get());
}

void CDMInstanceSessionProxyFbxcdm::challengeGeneratedCallback(CDMInstanceSession::MessageType messageType, Ref<SharedBuffer>&& message)
{
    if (!m_challengeCallbacks.isEmpty()) {
        for (const auto& challengeCallback : m_challengeCallbacks)
            challengeCallback(messageType, WTFMove(message));

        m_challengeCallbacks.clear();
#if 0
	} else if (!m_sessionChangedCallbacks.isEmpty()) {
		for (auto& sessionChangedCallback : m_sessionChangedCallbacks)
			sessionChangedCallback(true, parsedResponseMessage.payload().copyRef());

		m_sessionChangedCallbacks.clear();
#endif
    } else {
        if (m_client) {
            m_client->sendMessage(messageType, WTFMove(message));
        }
    }
}

void CDMInstanceSessionProxyFbxcdm::keyUpdatedCallback(KeyIDType&& keyID, CDMInstanceSession::KeyStatus keyStatus)
{
    Vector<uint8_t> dummy;
    m_doesKeyStoreNeedMerging |= m_keyStore.add(KeyHandle::create(keyStatus, WTFMove(keyID), WTFMove(dummy)));
}

void CDMInstanceSessionProxyFbxcdm::keysUpdateDoneCallback()
{
    if (m_doesKeyStoreNeedMerging) {
        m_doesKeyStoreNeedMerging = false;

        if (auto instanceProxyFbxcdm = cdmInstanceProxyFbxcdm())
            instanceProxyFbxcdm->mergeKeysFrom(m_keyStore);
    }

    if (m_sessionChangedCallbacks.isEmpty() && m_client) {
        m_client->updateKeyStatuses(m_keyStore.convertToJSKeyStatusVector());
        return;
    }

    for (auto& sessionChangedCallback : m_sessionChangedCallbacks)
        sessionChangedCallback(true, nullptr);

    m_sessionChangedCallbacks.clear();
}

void CDMInstanceSessionProxyFbxcdm::sessionFailure()
{
    for (auto& sessionChangedCallback : m_sessionChangedCallbacks)
        sessionChangedCallback(false, nullptr);

    m_sessionChangedCallbacks.clear();
}

void CDMInstanceSessionProxyFbxcdm::setClient(WeakPtr<CDMInstanceSessionClient>&& client)
{
    m_client = WTFMove(client);
}

void CDMInstanceSessionProxyFbxcdm::clearClient()
{
    m_client.clear();
}

void CDMInstanceSessionProxyFbxcdm::requestLicense(LicenseType licenseType, const AtomString& initDataType, Ref<SharedBuffer>&& initData, LicenseCallback&& callback)
{
    ASSERT(isMainThread());

    auto instanceProxyFbxcdm = cdmInstanceProxyFbxcdm();
    ASSERT(instanceProxyFbxcdm);

    auto& keys = instanceProxyFbxcdm->fbxcdmKeys();

    String empty_sessionID {};
    m_sessionID = empty_sessionID;

    m_session = keys.createSession(licenseType);
    if (!m_session) {
        callback(SharedBuffer::create(), {}, false, Failed);
        return;
    }

    m_challengeCallbacks.append([this, callback = WTFMove(callback)](CDMInstanceSession::MessageType messageType, Ref<SharedBuffer>&& message) mutable {
        ASSERT(isMainThread());

        if (m_sessionID.isEmpty()) {
            callback(SharedBuffer::create(), {}, false, Failed);
            return;
        }

        callback(WTFMove(message), m_sessionID, messageType == CDMInstanceSession::MessageType::IndividualizationRequest, Succeeded);
    });

    m_session->setClient(*this);

    m_session->generateRequest(initDataType, WTFMove(initData));

    m_sessionID = m_session->getSessionId();
}

void CDMInstanceSessionProxyFbxcdm::updateLicense(const String& sessionID, LicenseType licenseType, Ref<SharedBuffer>&& response, LicenseUpdateCallback&& callback)
{
    UNUSED_PARAM(licenseType);
    ASSERT_UNUSED(sessionID, sessionID == m_sessionID);

    m_sessionChangedCallbacks.append([this, callback = WTFMove(callback)](bool success, RefPtr<SharedBuffer>&& responseMessage) mutable {
        ASSERT(isMainThread());

        if (success) {
            if (!responseMessage) {
                callback(false, m_keyStore.convertToJSKeyStatusVector(), std::nullopt, std::nullopt, SuccessValue::Succeeded);
            } else {
                // FIXME: Using JSON reponse messages is much cleaner than using string prefixes, I believe there
                // will even be other parts of the spec where not having structured data will be bad.
                ParsedResponseMessage parsedResponseMessage(responseMessage);
                ASSERT(parsedResponseMessage);

                if (parsedResponseMessage.hasPayload()) {
                    Ref<SharedBuffer> message = WTFMove(parsedResponseMessage.payload());
                    callback(false, std::nullopt, std::nullopt, std::make_pair(parsedResponseMessage.typeOr(MediaKeyMessageType::LicenseRequest), WTFMove(message)), SuccessValue::Succeeded);
                } else {
                    callback(false, std::nullopt, std::nullopt, std::nullopt, SuccessValue::Failed);
                }
            }
        } else {
            callback(false, std::nullopt, std::nullopt, std::nullopt, SuccessValue::Failed);
        }
    });

    if (!m_session || m_sessionID.isEmpty() || m_session->update(WTFMove(response)))
        sessionFailure();
}

void CDMInstanceSessionProxyFbxcdm::loadSession(LicenseType licenseType, const String& sessionID, const String& origin, LoadSessionCallback&& callback)
{
    UNUSED_PARAM(licenseType);
    UNUSED_PARAM(origin);

    m_sessionChangedCallbacks.append([this, callback = WTFMove(callback)](bool success, RefPtr<SharedBuffer>&& responseMessage) mutable {
        ASSERT(isMainThread());

        if (success) {
            if (!responseMessage) {
                callback(m_keyStore.convertToJSKeyStatusVector(), std::nullopt, std::nullopt, SuccessValue::Succeeded, SessionLoadFailure::None);
            } else {
                // FIXME: Using JSON reponse messages is much cleaner than using string prefixes, I believe there
                // will even be other parts of the spec where not having structured data will be bad.
                ParsedResponseMessage parsedResponseMessage(responseMessage);
                ASSERT(parsedResponseMessage);

                if (parsedResponseMessage.hasPayload()) {
                    Ref<SharedBuffer> message = WTFMove(parsedResponseMessage.payload());
                    callback(std::nullopt, std::nullopt, std::make_pair(parsedResponseMessage.typeOr(MediaKeyMessageType::LicenseRequest), WTFMove(message)), SuccessValue::Succeeded, SessionLoadFailure::None);
                } else {
                    callback(std::nullopt, std::nullopt, std::nullopt, SuccessValue::Failed, SessionLoadFailure::Other);
                }
            }
        } else {
            auto responseMessageData = responseMessage ? responseMessage->data() : nullptr;
            auto responseMessageSize = responseMessage ? responseMessage->size() : 0;
            StringView response(reinterpret_cast<const LChar*>(responseMessageData), responseMessageSize);
            callback(std::nullopt, std::nullopt, std::nullopt, SuccessValue::Failed, sessionLoadFailureFromFbxcdm(response));
        }
    });

    if (!m_session || m_sessionID.isEmpty() || m_session->load(sessionID))
        sessionFailure();
}

void CDMInstanceSessionProxyFbxcdm::closeSession(const String& sessionID, CloseSessionCallback&& callback)
{
    ASSERT_UNUSED(sessionID, m_sessionID == sessionID);

    if (m_session && !m_sessionID.isEmpty()) {
        m_session->close();

        m_session->clearClient();

        if (auto instanceProxyFbxcdm = cdmInstanceProxyFbxcdm())
            instanceProxyFbxcdm->unrefAllKeysFrom(m_keyStore);

        m_keyStore.unrefAllKeys();
    }

    callback();
}

void CDMInstanceSessionProxyFbxcdm::removeSessionData(const String& sessionID, LicenseType licenseType, RemoveSessionDataCallback&& callback)
{
    UNUSED_PARAM(licenseType);
    ASSERT_UNUSED(sessionID, m_sessionID == sessionID);

    m_sessionChangedCallbacks.append([this, callback = WTFMove(callback)](bool success, RefPtr<SharedBuffer>&& buffer) mutable {
        ASSERT(isMainThread());

        if (success) {
            if (!buffer) {
                callback(m_keyStore.allKeysAs(MediaKeyStatus::Released), nullptr, SuccessValue::Succeeded);
            } else {
                ParsedResponseMessage parsedResponseMessage(buffer);
                ASSERT(parsedResponseMessage);

                if (parsedResponseMessage.hasPayload()) {
                    Ref<SharedBuffer> message = WTFMove(parsedResponseMessage.payload());
                    callback(m_keyStore.allKeysAs(MediaKeyStatus::Released), WTFMove(message), SuccessValue::Succeeded);
                } else {
                    callback(m_keyStore.allKeysAs(MediaKeyStatus::InternalError), nullptr, SuccessValue::Failed);
                }
            }
        } else {
            callback(m_keyStore.allKeysAs(MediaKeyStatus::InternalError), nullptr, SuccessValue::Failed);
        }
    });

    if (!m_session || m_sessionID.isEmpty() || m_session->remove())
        sessionFailure();
}

void CDMInstanceSessionProxyFbxcdm::storeRecordOfKeyUsage(const String& sessionId)
{
    UNUSED_PARAM(sessionId);
    notImplemented();
}

} // namespace WebCore

#endif // ENABLE(ENCRYPTED_MEDIA) && ENABLE(FBXCDM)
