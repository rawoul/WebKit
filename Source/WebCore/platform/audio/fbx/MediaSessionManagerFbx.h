#pragma once

#if USE(GLIB) && ENABLE(MEDIA_SESSION) && USE(FBX_API)

#include "MediaSessionIdentifier.h"
#include "NowPlayingManager.h"
#include "PlatformMediaSessionManager.h"
#include "MediaSessionFbx.h"
#include <wtf/MainThread.h>
#include <wtf/Fbxbus.h>

struct mediacontroller_signal;

namespace WebCore {

struct NowPlayingInfo;
class MediaSessionFbx;

class MediaSessionManagerFbx
    : public PlatformMediaSessionManager
    ,private NowPlayingManager::Client {
    WTF_MAKE_FAST_ALLOCATED;
public:
    MediaSessionManagerFbx();
    ~MediaSessionManagerFbx();

    struct mediacontroller_signal *mediactl_sig;

    void beginInterruption(PlatformMediaSession::InterruptionType) final;
    void dispatch(PlatformMediaSession::RemoteControlCommandType, PlatformMediaSession::RemoteCommandArgument);
    PlatformMediaSession* nowPlayingEligibleSession();
    HashMap<MediaSessionIdentifier, std::unique_ptr<MediaSessionFbx>> m_sessions;

protected:
    void scheduleSessionStatusUpdate() final;
    void updateNowPlayingInfo();

    void removeSession(PlatformMediaSession&) final;
    void addSession(PlatformMediaSession&) final;
    void setCurrentSession(PlatformMediaSession&) final;

    bool sessionWillBeginPlayback(PlatformMediaSession&) override;
    void sessionWillEndPlayback(PlatformMediaSession&, DelayCallingUpdateNowPlaying) override;
    void sessionStateChanged(PlatformMediaSession&) override;
    void sessionDidEndRemoteScrubbing(PlatformMediaSession&) final;
    void clientCharacteristicsChanged(PlatformMediaSession&);
    void sessionCanProduceAudioChanged() final;

    virtual void providePresentingApplicationPIDIfNecessary() { }

    void addSupportedCommand(PlatformMediaSession::RemoteControlCommandType) final;
    void removeSupportedCommand(PlatformMediaSession::RemoteControlCommandType) final;
    RemoteCommandListener::RemoteCommandsSet supportedCommands() const final;

    void resetHaveEverRegisteredAsNowPlayingApplicationForTesting() final { m_haveEverRegisteredAsNowPlayingApplication = false; };

private:
    // NowPlayingManager::Client
    void didReceiveRemoteControlCommand(PlatformMediaSession::RemoteControlCommandType type, const PlatformMediaSession::RemoteCommandArgument& argument) final { processDidReceiveRemoteControlCommand(type, argument); }

    bool m_isSeeking { false };
    bool m_nowPlayingActive { false };
    bool m_registeredAsNowPlayingApplication { false };
    bool m_haveEverRegisteredAsNowPlayingApplication { false };

    // For testing purposes only.
    String m_lastUpdatedNowPlayingTitle;
    double m_lastUpdatedNowPlayingDuration { NAN };
    double m_lastUpdatedNowPlayingElapsedTime { NAN };
    MediaUniqueIdentifier m_lastUpdatedNowPlayingInfoUniqueIdentifier;

    const std::unique_ptr<NowPlayingManager> m_nowPlayingManager;
};

} // namespace WebCore

#endif // USE(GLIB) && ENABLE(MEDIA_SESSION) && USE(FBX_API)
