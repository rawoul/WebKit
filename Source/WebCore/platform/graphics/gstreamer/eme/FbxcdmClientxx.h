#pragma once

#if ENABLE(ENCRYPTED_MEDIA) && ENABLE(FBXCDM)

#include <fbxcdm.h>

#include "CDMInstanceSession.h"

namespace WebCore {

class CDMInstanceSessionProxyFbxcdm;

using KeyIDType = Vector<uint8_t>;

namespace Fbxcdm {

class Client;
class MediaKeySystemAccess;
class MediaKeys;
class MediaKeySession;

class Client {
    WTF_MAKE_FAST_ALLOCATED;

public:
    static Client& singleton();

    WeakPtr<MediaKeySystemAccess> requestMediaKeySystemAccess(const String& keySystem);
    void refSession(int session_id, WeakPtr<MediaKeySession>&& session);
    void unrefSession(int session_id);

private:
    static void cb_session_key_statuses_change(void* priv, int session_id, const struct fbxcdm_key_status_map* key_statuses);
    static void cb_session_message(void* priv, int session_id, enum fbxcdm_key_msg_type type, const char* content);
    static void cb_session_closed(void* priv, int session_id);

    Client();
    void onKeyStatusesChange(int session_id, const struct fbxcdm_key_status_map* key_statuses);
    void onMessage(int session_id, enum fbxcdm_key_msg_type type, const char* content);
    void onClosed(int session_id);

    Client(const Client&) = delete;
    void operator=(const Client&) = delete;
    Client(Client&&) = delete;
    Client& operator=(Client&&) = delete;
    ~Client() = delete;

    friend class LazyNeverDestroyed<Client>;

    HashMap<String, RefPtr<MediaKeySystemAccess>> m_mksaMap;
    HashMap<int, WeakPtr<MediaKeySession>> m_sessionMap;
    struct fbxcdm_signal* m_sig_message;
    struct fbxcdm_signal* m_sig_key_statuses_change;
    struct fbxcdm_signal* m_sig_closed;
};

class MediaKeySystemAccess final : public RefCounted<MediaKeySystemAccess>, public CanMakeWeakPtr<MediaKeySystemAccess> {
    WTF_MAKE_FAST_ALLOCATED;

public:
    static RefPtr<MediaKeySystemAccess> create(int mksa_id, const String& keySystem);

    ~MediaKeySystemAccess();
    const String& keySystem() const;
    bool supportsContentType(const String& contentType) const;
    RefPtr<MediaKeys> createMediaKeys();

private:
    MediaKeySystemAccess(int mksa_id, const String& keySystem);

    MediaKeySystemAccess() = delete;
    MediaKeySystemAccess(const MediaKeySystemAccess&) = delete;
    void operator=(const MediaKeySystemAccess&) = delete;
    MediaKeySystemAccess(MediaKeySystemAccess&&) = delete;
    MediaKeySystemAccess& operator=(MediaKeySystemAccess&&) = delete;

    int m_mksa_id;
    String m_keySystem;
};

class MediaKeys final : public RefCounted<MediaKeys>, public CanMakeWeakPtr<MediaKeys> {
    WTF_MAKE_FAST_ALLOCATED;

public:
    static RefPtr<MediaKeys> create(int keys_id);

    ~MediaKeys();
    int id() const;
    int setOrigin(const String& origin);
    bool setServerCertificate(Ref<SharedBuffer>&& certificate);
    RefPtr<MediaKeySession> createSession(CDMInstanceSession::LicenseType licenseType);

private:
    MediaKeys(int keys_id);

    MediaKeys() = delete;
    MediaKeys(const MediaKeys&) = delete;
    void operator=(const MediaKeys&) = delete;
    MediaKeys(MediaKeys&&) = delete;
    MediaKeys& operator=(MediaKeys&&) = delete;

    int m_keys_id;
};

class MediaKeySession final : public RefCounted<MediaKeySession>, public CanMakeWeakPtr<MediaKeySession> {
    WTF_MAKE_FAST_ALLOCATED;

public:
    static RefPtr<MediaKeySession> create(int session_id, const enum fbxcdm_session_type& sessionType);

    ~MediaKeySession();
    void setClient(WeakPtr<CDMInstanceSessionProxyFbxcdm>&& client);
    void clearClient();
    const String& getSessionId() const;
    int generateRequest(const AtomString& initDataType, Ref<SharedBuffer>&& initData);
    int load(const String& sessionID);
    int update(Ref<SharedBuffer>&& response);
    int close();
    int remove();
    void onKeyStatusesChange(const struct fbxcdm_key_status_map* key_statuses);
    void onMessage(enum fbxcdm_key_msg_type type, const char* content);
    void onClosed();

private:
    MediaKeySession(int session_id, const enum fbxcdm_session_type& sessionType);
    void sendNotifKeyStatusesChange();

    MediaKeySession() = delete;
    MediaKeySession(const MediaKeySession&) = delete;
    void operator=(const MediaKeySession&) = delete;
    MediaKeySession(MediaKeySession&&) = delete;
    MediaKeySession& operator=(MediaKeySession&&) = delete;

    int m_session_id;
    enum fbxcdm_session_type m_sessionType;
    String m_id_string;
    HashMap<String, CDMInstanceSession::KeyStatus> m_keyStatusesMap;
    WeakPtr<CDMInstanceSessionProxyFbxcdm> m_client;
};

} // namespace Fbxcdm

} // namespace WebCore

#endif // ENABLE(ENCRYPTED_MEDIA) && ENABLE(FBXCDM)
