#include "config.h"
#include "FbxcdmClientxx.h"

#if ENABLE(ENCRYPTED_MEDIA) && ENABLE(FBXCDM)

#include <wtf/Fbxbus.h>
#include <wtf/text/Base64.h>

#include <fbxcdm.h>

#include "CDMFbxcdm.h"
#include "Logging.h"

namespace WebCore {

namespace Fbxcdm {

// Client singleton ===========================================================

Client& Client::singleton()
{
    ASSERT(isMainThread());

    static LazyNeverDestroyed<Client> client;
    static std::once_flag onceKey;
    std::call_once(onceKey, [&] {
        client.construct();
    });
    return client;
}

Client::Client()
{
    ASSERT(isMainThread());

    m_sig_message = fbxcdm_bind_session_message(Fbxbus::singleton(), cb_session_message, this);
    m_sig_key_statuses_change = fbxcdm_bind_session_key_statuses_change(Fbxbus::singleton(), cb_session_key_statuses_change, this);
    m_sig_closed = fbxcdm_bind_session_closed(Fbxbus::singleton(), cb_session_closed, this);
}

void Client::cb_session_key_statuses_change(void* priv, int session_id, const struct fbxcdm_key_status_map* key_statuses)
{
    ASSERT(isMainThread());

    Client* client = (Client*)priv;

    client->onKeyStatusesChange(session_id, key_statuses);
}

void Client::cb_session_message(void* priv, int session_id, enum fbxcdm_key_msg_type type, const char* content)
{
    ASSERT(isMainThread());

    Client* client = (Client*)priv;

    client->onMessage(session_id, type, content);
}

void Client::cb_session_closed(void* priv, int session_id)
{
    ASSERT(isMainThread());

    Client* client = (Client*)priv;

    client->onClosed(session_id);
}

void Client::refSession(int session_id, WeakPtr<MediaKeySession>&& s)
{
    ASSERT(isMainThread());

    auto addResult = m_sessionMap.set(session_id, nullptr);

    auto& it = addResult.iterator;

    auto& sessionRef = it->value;

    sessionRef = WTFMove(s);
}

void Client::unrefSession(int session_id)
{
    ASSERT(isMainThread());

    auto it = m_sessionMap.find(session_id);
    if (it == m_sessionMap.end())
        return;

    auto& sessionRef = it->value;

    sessionRef.clear();

    m_sessionMap.remove(it);
}

void Client::onKeyStatusesChange(int session_id, const struct fbxcdm_key_status_map* key_statuses)
{
    ASSERT(isMainThread());

    auto it = m_sessionMap.find(session_id);
    if (it == m_sessionMap.end())
        return;

    auto& sessionRef = it->value;

    if (sessionRef)
        sessionRef->onKeyStatusesChange(key_statuses);
}

void Client::onMessage(int session_id, enum fbxcdm_key_msg_type type, const char* content)
{
    ASSERT(isMainThread());

    auto it = m_sessionMap.find(session_id);
    if (it == m_sessionMap.end())
        return;

    auto& sessionRef = it->value;

    if (sessionRef)
        sessionRef->onMessage(type, content);
}

void Client::onClosed(int session_id)
{
    ASSERT(isMainThread());

    auto it = m_sessionMap.find(session_id);
    if (it == m_sessionMap.end())
        return;

    auto& sessionRef = it->value;

    if (sessionRef)
        sessionRef->onClosed();
}

WeakPtr<MediaKeySystemAccess> Client::requestMediaKeySystemAccess(const String& keySystem)
{
    ASSERT(isMainThread());

    auto it = m_mksaMap.find(keySystem);
    if (it != m_mksaMap.end()) {
        auto& validMksaRef = it->value;
        if (validMksaRef)
            return *validMksaRef.get();
    }

    int system_id;
    int r = fbxcdm_request_system(Fbxbus::singleton(), keySystem.utf8().data(), &system_id);
    if (r != FBXCDM_SUCCESS)
        return nullptr;

    auto mksaRef = MediaKeySystemAccess::create(system_id, keySystem);

    auto addResult = m_mksaMap.set(keySystem, nullptr);

    auto& iterator = addResult.iterator;

    auto& ref = iterator->value;

    ref = WTFMove(mksaRef);

    return *ref.get();
}

// MediaKeySystemAccess =======================================================

RefPtr<MediaKeySystemAccess> MediaKeySystemAccess::create(int mksa_id, const String& keySystem)
{
    ASSERT(isMainThread());

    return adoptRef(new MediaKeySystemAccess(mksa_id, keySystem));
}

MediaKeySystemAccess::MediaKeySystemAccess(int mksa_id, const String& keySystem)
    : m_mksa_id(mksa_id)
    , m_keySystem(keySystem)
{
    ASSERT(isMainThread());

    RELEASE_LOG(EME, "Fbxcdm::MediaKeySystemAccess@%d.new()", m_mksa_id);
}

MediaKeySystemAccess::~MediaKeySystemAccess()
{
    ASSERT(isMainThread());

    RELEASE_LOG(EME, "Fbxcdm::MediaKeySystemAccess@%d.delete()", m_mksa_id);
}

const String& MediaKeySystemAccess::keySystem() const
{
    ASSERT(isMainThread());

    return m_keySystem;
}

bool MediaKeySystemAccess::supportsContentType(const String& contentType) const
{
    ASSERT(isMainThread());

    RELEASE_LOG(EME, "Fbxcdm::MediaKeySystemAccess@%d.supportsContentType(contentType:%s)", m_mksa_id, contentType.utf8().data());

    return true;
}

RefPtr<MediaKeys> MediaKeySystemAccess::createMediaKeys()
{
    ASSERT(isMainThread());

    RefPtr<MediaKeys> invalidKeysRef;

    int keys_id;
    int r = fbxcdm_system_create_keys(Fbxbus::singleton(), m_mksa_id, &keys_id);
    if (r != FBXCDM_SUCCESS)
        return invalidKeysRef;

    return MediaKeys::create(keys_id);
}

// MediaKeys ==================================================================

RefPtr<MediaKeys> MediaKeys::create(int keys_id)
{
    ASSERT(isMainThread());

    return adoptRef(new MediaKeys(keys_id));
}

MediaKeys::MediaKeys(int keys_id)
    : m_keys_id(keys_id)
{
    ASSERT(isMainThread());

    RELEASE_LOG(EME, "Fbxcdm::MediaKeys@%d.new()", m_keys_id);
}

MediaKeys::~MediaKeys()
{
    ASSERT(isMainThread());

    RELEASE_LOG(EME, "Fbxcdm::MediaKeys@%d.delete()", m_keys_id);

    fbxcdm_keys_destroy(Fbxbus::singleton(), m_keys_id);
}

int MediaKeys::id() const
{
    // not on main thread

    return m_keys_id;
}

int MediaKeys::setOrigin(const String& origin)
{
    ASSERT(isMainThread());

    RELEASE_LOG(EME, "Fbxcdm::MediaKeys@%d.setOrigin(origin:%s)", m_keys_id, origin.utf8().data());

    int r = fbxcdm_keys_set_origin(Fbxbus::singleton(), m_keys_id, origin.utf8().data());
    if (r != FBXCDM_SUCCESS)
        return -1;

    return 0;
}

bool MediaKeys::setServerCertificate(Ref<SharedBuffer>&& certificate)
{
    ASSERT(isMainThread());

    auto data = certificate->extractData();
    auto vec = base64EncodeToVector(const_cast<uint8_t*>(data.data()), data.size());
    vec.append(0);
    const char* str = (const char*)vec.data();

    RELEASE_LOG(EME, "Fbxcdm::MediaKeys@%d.setServerCertificate(serverCertificate:%s)", m_keys_id, str);

    bool boolret;
    int r = fbxcdm_keys_set_srv_cert(Fbxbus::singleton(), m_keys_id, str, &boolret);
    if (r != FBXCDM_SUCCESS)
        return false;

    return boolret;
}

RefPtr<MediaKeySession> MediaKeys::createSession(CDMInstanceSession::LicenseType licenseType)
{
    ASSERT(isMainThread());

    RefPtr<MediaKeySession> sessionRef;

    enum fbxcdm_session_type sessionType;
    switch (licenseType) {
    case CDMInstanceSession::LicenseType::Temporary:
        sessionType = FBXCDM_SESSION_TYPE_TEMPORARY;
        break;

    case CDMInstanceSession::LicenseType::PersistentLicense:
        sessionType = FBXCDM_SESSION_TYPE_PERSISTENT_LICENSE;
        break;

    default:
        ASSERT_NOT_REACHED();
        return sessionRef;
    }

    int session_id;
    int r = fbxcdm_keys_create_session(Fbxbus::singleton(), m_keys_id, sessionType, &session_id);
    if (r != FBXCDM_SUCCESS)
        return sessionRef;

    sessionRef = MediaKeySession::create(session_id, sessionType);

    Client::singleton().refSession(session_id, *sessionRef.get());

    return sessionRef;
}

// MediaKeySession ============================================================

RefPtr<MediaKeySession> MediaKeySession::create(int session_id, const enum fbxcdm_session_type& sessionType)
{
    ASSERT(isMainThread());

    return adoptRef(new MediaKeySession(session_id, sessionType));
}

MediaKeySession::MediaKeySession(int session_id, const enum fbxcdm_session_type& sessionType)
    : m_session_id(session_id)
    , m_sessionType(sessionType)
    , m_id_string("FakeMediaKeySessionId" + String::number(session_id))
    , m_keyStatusesMap()
    , m_client()
{
    ASSERT(isMainThread());

    RELEASE_LOG(EME, "Fbxcdm::MediaKeySession@%d.new()", m_session_id);
}

MediaKeySession::~MediaKeySession()
{
    ASSERT(isMainThread());

    RELEASE_LOG(EME, "Fbxcdm::MediaKeySession@%d.delete()", m_session_id);

    Client::singleton().unrefSession(m_session_id);

    fbxcdm_session_destroy(Fbxbus::singleton(), m_session_id);
}

void MediaKeySession::setClient(WeakPtr<CDMInstanceSessionProxyFbxcdm>&& client)
{
    ASSERT(isMainThread());

    m_client = WTFMove(client);
}

void MediaKeySession::clearClient()
{
    ASSERT(isMainThread());

    m_client.clear();
}

const String& MediaKeySession::getSessionId() const
{
    ASSERT(isMainThread());

    return m_id_string;
}

int MediaKeySession::generateRequest(const AtomString& initDataType, Ref<SharedBuffer>&& initData)
{
    ASSERT(isMainThread());

    enum fbxcdm_init_data_type init_data_type;
    if (equalLettersIgnoringASCIICase(initDataType, "cenc"_s)) {
        init_data_type = FBXCDM_INIT_DATA_TYPE_CENC;
    } else if (equalLettersIgnoringASCIICase(initDataType, "keyids"_s)) {
        init_data_type = FBXCDM_INIT_DATA_TYPE_KEYIDS;
    } else if (equalLettersIgnoringASCIICase(initDataType, "webm"_s)) {
        init_data_type = FBXCDM_INIT_DATA_TYPE_WEBM;
    } else {
        ASSERT_NOT_REACHED();
        return -1;
    }

    auto data = initData->extractData();
    auto vec = base64EncodeToVector(const_cast<uint8_t*>(data.data()), data.size());
    vec.append(0);
    const char* str = (const char*)vec.data();

    RELEASE_LOG(EME, "Fbxcdm::MediaKeySession@%d.generateRequest(initData:%s)", m_session_id, str);

    int r = fbxcdm_session_generate_request(Fbxbus::singleton(), m_session_id, init_data_type, str);
    if (r != FBXCDM_SUCCESS)
        return -1;

    return 0;
}

int MediaKeySession::load(const String& sessionID)
{
    ASSERT(isMainThread());

    RELEASE_LOG(EME, "Fbxcdm::MediaKeySession@%d.load(sessionID:%s)", m_session_id, sessionID.utf8().data());

    bool b;
    int r = fbxcdm_session_load(Fbxbus::singleton(), m_session_id, sessionID.utf8().data(), &b);
    if (r != FBXCDM_SUCCESS)
        return -1;

    if (!b)
        return -1;

    return 0;
}

int MediaKeySession::update(Ref<SharedBuffer>&& response)
{
    ASSERT(isMainThread());

    auto data = response->extractData();
    auto vec = base64EncodeToVector(const_cast<uint8_t*>(data.data()), data.size());
    vec.append(0);
    const char* str = (const char*)vec.data();

    RELEASE_LOG(EME, "Fbxcdm::MediaKeySession@%d.update(reponse:%s)", m_session_id, str);

    int r = fbxcdm_session_update(Fbxbus::singleton(), m_session_id, str);
    if (r != FBXCDM_SUCCESS)
        return -1;

    // workaround for appletvplus:
    if (!m_keyStatusesMap.isEmpty())
        sendNotifKeyStatusesChange();

    return 0;
}

int MediaKeySession::close()
{
    ASSERT(isMainThread());

    RELEASE_LOG(EME, "Fbxcdm::MediaKeySession@%d.close()", m_session_id);

    int r = fbxcdm_session_close(Fbxbus::singleton(), m_session_id);
    if (r != FBXCDM_SUCCESS)
        return -1;

    return 0;
}

int MediaKeySession::remove()
{
    ASSERT(isMainThread());

    RELEASE_LOG(EME, "Fbxcdm::MediaKeySession@%d.remove()", m_session_id);

    int r = fbxcdm_session_remove(Fbxbus::singleton(), m_session_id);
    if (r != FBXCDM_SUCCESS)
        return -1;

    return 0;
}

void MediaKeySession::sendNotifKeyStatusesChange()
{
    ASSERT(isMainThread());

    for (const auto& [kid, keyStatus] : m_keyStatusesMap) {
        auto data = base64Decode(kid);

        KeyIDType keyID;
        keyID.append(data->data(), data->size());

        if (m_client)
            m_client->keyUpdatedCallback(WTFMove(keyID), keyStatus);
    }

    if (m_client)
        m_client->keysUpdateDoneCallback();
}

void MediaKeySession::onKeyStatusesChange(const struct fbxcdm_key_status_map* key_statuses)
{
    ASSERT(isMainThread());

    RELEASE_LOG(EME, "Fbxcdm::MediaKeySession@%d.onKeyStatusesChange()", m_session_id);

    HashMap<String, CDMInstanceSession::KeyStatus> newkeyStatusesMap;

    for (struct fbxcdm_key_status_it* it = &key_statuses->map[0];
         it < &key_statuses->map[key_statuses->size_map];
         ++it) {

        String keyID(it->key_id, strlen(it->key_id));

        CDMInstanceSession::KeyStatus status;
        switch (it->status) {
        case FBXCDM_KEY_STATUS_USABLE:
            status = CDMInstanceSession::KeyStatus::Usable;
            break;

        case FBXCDM_KEY_STATUS_EXPIRED:
            status = CDMInstanceSession::KeyStatus::Expired;
            break;

        case FBXCDM_KEY_STATUS_RELEASED:
            status = CDMInstanceSession::KeyStatus::Released;
            break;

        case FBXCDM_KEY_STATUS_OUTPUT_RESTRICTED:
            status = CDMInstanceSession::KeyStatus::OutputRestricted;
            break;

        case FBXCDM_KEY_STATUS_OUTPUT_DOWNSCALED:
            status = CDMInstanceSession::KeyStatus::OutputDownscaled;
            break;

        case FBXCDM_KEY_STATUS_STATUS_PENDING:
            status = CDMInstanceSession::KeyStatus::StatusPending;
            break;

        case FBXCDM_KEY_STATUS_INTERNAL_ERROR:
            status = CDMInstanceSession::KeyStatus::InternalError;
            break;
        }

        newkeyStatusesMap.set(keyID, status);
    }

    if (newkeyStatusesMap == m_keyStatusesMap)
        return;

    m_keyStatusesMap = newkeyStatusesMap;

    sendNotifKeyStatusesChange();
}

void MediaKeySession::onMessage(enum fbxcdm_key_msg_type type, const char* content)
{
    ASSERT(isMainThread());

    RELEASE_LOG(EME, "Fbxcdm::MediaKeySession@%d.onMessage(message:%s)", m_session_id, content);

    auto data = base64Decode(content, strlen(content));
    if (!data)
        return;

    auto buffer = SharedBuffer::create(data->data(), data->size());

    CDMInstanceSession::MessageType messageType;
    switch (type) {
    case FBXCDM_KEY_MSG_TYPE_LICENSE_REQUEST:
        messageType = CDMInstanceSession::MessageType::LicenseRequest;
        break;
    case FBXCDM_KEY_MSG_TYPE_LICENSE_RENEWAL:
        messageType = CDMInstanceSession::MessageType::LicenseRenewal;
        break;
    case FBXCDM_KEY_MSG_TYPE_LICENSE_RELEASE:
        messageType = CDMInstanceSession::MessageType::LicenseRelease;
        break;
    case FBXCDM_KEY_MSG_TYPE_INDIV_REQUEST:
        messageType = CDMInstanceSession::MessageType::IndividualizationRequest;
        break;
    }

    if (m_client)
        m_client->challengeGeneratedCallback(messageType, WTFMove(buffer));
}

void MediaKeySession::onClosed()
{
    ASSERT(isMainThread());

    RELEASE_LOG(EME, "Fbxcdm::MediaKeySession@%d.onClose()", m_session_id);
}

} // namespace Fbxcdm

} // namespace WebCore

#endif // ENABLE(ENCRYPTED_MEDIA) && ENABLE(FBXCDM)
