#include "config.h"
#include "MediaSessionManagerFbx.h"

#if USE(GLIB) && ENABLE(MEDIA_SESSION) && USE(FBX_API)

#include "HTMLMediaElement.h"
#include "MediaPlayer.h"
#include "MediaStrategy.h"
#include "NowPlayingInfo.h"
#include "PlatformMediaSession.h"
#include "PlatformStrategies.h"
#include "MediaSessionIdentifier.h"
#include <wtf/SortedArrayMap.h>

#include <mediacontroller.h>

namespace WebCore {

static std::optional<PlatformMediaSession::RemoteControlCommandType> getCommand(enum mediacontroller_media_cmd cmd)
{
    static const std::pair<mediacontroller_media_cmd, PlatformMediaSession::RemoteControlCommandType> commandList[] = {
        { MEDIACONTROLLER_MEDIA_CMD_PLAY, PlatformMediaSession::PlayCommand },
        { MEDIACONTROLLER_MEDIA_CMD_PAUSE, PlatformMediaSession::PauseCommand },
        { MEDIACONTROLLER_MEDIA_CMD_PLAY_PAUSE, PlatformMediaSession::TogglePlayPauseCommand },
        { MEDIACONTROLLER_MEDIA_CMD_STOP, PlatformMediaSession::StopCommand },
        { MEDIACONTROLLER_MEDIA_CMD_NEXT, PlatformMediaSession::NextTrackCommand },
        { MEDIACONTROLLER_MEDIA_CMD_PREV, PlatformMediaSession::PreviousTrackCommand },
        { MEDIACONTROLLER_MEDIA_CMD_SEEK_TO, PlatformMediaSession::SeekToPlaybackPositionCommand },
    };

    static const SortedArrayMap map { commandList };
    auto value = map.get(cmd, PlatformMediaSession::RemoteControlCommandType::NoCommand);
    if (value == PlatformMediaSession::RemoteControlCommandType::NoCommand)
        return { };
    return value;
}

static void mediactrl_cmd_process(void *priv,
                  const char *target,
                  enum mediacontroller_media_cmd cmd,
                  const union mediacontroller_media_cmd_arg *args,
                  enum mediacontroller_type_media_cmd_arg type_args,
                  enum mediacontroller_media_cmd_source source)
{
    UNUSED_PARAM(source);

    WTFLogAlways("mediacontroller: process command %s for %s",
            mediacontroller_media_cmd_to_str(cmd), target);

    auto& manager = *reinterpret_cast<MediaSessionManagerFbx*>(priv);

    auto command = getCommand(cmd);
    if (!command) {
        WTFLogAlways("mediacontroller: command not found");
        return;
    }

    PlatformMediaSession::RemoteCommandArgument argument;
    if (*command == PlatformMediaSession::SeekToPlaybackPositionCommand) {
        if (type_args == MEDIACONTROLLER_U_MEDIA_CMD_ARG_SEEK_POSITION ) {
            auto* platformSession = manager.nowPlayingEligibleSession();
            auto nowPlayingInfo = platformSession->nowPlayingInfo();
            double currentTime = nowPlayingInfo->currentTime;
            argument.time = args->seek_position.position / 1000.0 + currentTime;
        }
    }

    manager.dispatch(*command, argument);
}

std::unique_ptr<PlatformMediaSessionManager> PlatformMediaSessionManager::create()
{
    return std::unique_ptr<MediaSessionManagerFbx>(new MediaSessionManagerFbx);
}

MediaSessionManagerFbx::MediaSessionManagerFbx()
    : m_nowPlayingManager(platformStrategies()->mediaStrategy().createNowPlayingManager())
{
    mediactl_sig = mediacontroller_bind_media_cmd_process(Fbxbus::singleton(),
                                                        mediactrl_cmd_process,
                                                        this);
}

MediaSessionManagerFbx::~MediaSessionManagerFbx()
{
    if (mediactl_sig)
        mediacontroller_unbind(Fbxbus::singleton(), mediactl_sig);

}

void MediaSessionManagerFbx::beginInterruption(PlatformMediaSession::InterruptionType type)
{
    if (type == PlatformMediaSession::InterruptionType::SystemInterruption) {
        forEachSession([] (auto& session) {
            session.clearHasPlayedSinceLastInterruption();
        });
    }

    PlatformMediaSessionManager::beginInterruption(type);
}

void MediaSessionManagerFbx::scheduleSessionStatusUpdate()
{
    callOnMainThread([this] () mutable {
        m_nowPlayingManager->setSupportsSeeking(computeSupportsSeeking());
        updateNowPlayingInfo();

        forEachSession([] (auto& session) {
            session.updateMediaUsageIfChanged();
        });
    });
}

bool MediaSessionManagerFbx::sessionWillBeginPlayback(PlatformMediaSession& session)
{
    if (!PlatformMediaSessionManager::sessionWillBeginPlayback(session))
        return false;

    scheduleSessionStatusUpdate();
    return true;
}

void MediaSessionManagerFbx::sessionDidEndRemoteScrubbing(PlatformMediaSession&)
{
    scheduleSessionStatusUpdate();
}

void MediaSessionManagerFbx::addSession(PlatformMediaSession& platformSession)
{
    auto identifier = platformSession.mediaSessionIdentifier();
    auto session = MediaSessionFbx::create(*this, identifier);
    if (!session)
        return;

    m_sessions.add(identifier, WTFMove(session));
    m_nowPlayingManager->addClient(*this);

    PlatformMediaSessionManager::addSession(platformSession);
}

void MediaSessionManagerFbx::removeSession(PlatformMediaSession& session)
{
    PlatformMediaSessionManager::removeSession(session);
    m_sessions.remove(session.mediaSessionIdentifier());
    if (hasNoSession())
        m_nowPlayingManager->removeClient(*this);

    scheduleSessionStatusUpdate();
}

void MediaSessionManagerFbx::setCurrentSession(PlatformMediaSession& session)
{
    PlatformMediaSessionManager::setCurrentSession(session);
    m_nowPlayingManager->updateSupportedCommands();
}

void MediaSessionManagerFbx::sessionWillEndPlayback(PlatformMediaSession& session, DelayCallingUpdateNowPlaying delayCallingUpdateNowPlaying)
{
    PlatformMediaSessionManager::sessionWillEndPlayback(session, delayCallingUpdateNowPlaying);

    callOnMainThread([weakSession = WeakPtr { session }] {
        if (weakSession)
            weakSession->updateMediaUsageIfChanged();
    });

    if (delayCallingUpdateNowPlaying == DelayCallingUpdateNowPlaying::No)
        updateNowPlayingInfo();
    else {
        callOnMainThread([this] {
            updateNowPlayingInfo();
        });
    }
}

void MediaSessionManagerFbx::sessionStateChanged(PlatformMediaSession& platformSession)
{
    PlatformMediaSessionManager::sessionStateChanged(platformSession);
    auto session = m_sessions.get(platformSession.mediaSessionIdentifier());
    if (!session)
        return;

    session->playbackStatusChanged(platformSession);
}

void MediaSessionManagerFbx::clientCharacteristicsChanged(PlatformMediaSession& platformSession)
{
    if (m_isSeeking) {
        m_isSeeking = false;
        auto session = m_sessions.get(platformSession.mediaSessionIdentifier());
        session->emitPositionChanged(platformSession.nowPlayingInfo()->currentTime);
    }
    scheduleSessionStatusUpdate();
}

void MediaSessionManagerFbx::sessionCanProduceAudioChanged()
{
    ALWAYS_LOG(LOGIDENTIFIER);
    PlatformMediaSessionManager::sessionCanProduceAudioChanged();
    scheduleSessionStatusUpdate();
}

void MediaSessionManagerFbx::addSupportedCommand(PlatformMediaSession::RemoteControlCommandType command)
{
    m_nowPlayingManager->addSupportedCommand(command);
}

void MediaSessionManagerFbx::removeSupportedCommand(PlatformMediaSession::RemoteControlCommandType command)
{
    m_nowPlayingManager->removeSupportedCommand(command);
}

RemoteCommandListener::RemoteCommandsSet MediaSessionManagerFbx::supportedCommands() const
{
    return m_nowPlayingManager->supportedCommands();
}

PlatformMediaSession* MediaSessionManagerFbx::nowPlayingEligibleSession()
{
    // FIXME: Fix this layering violation.
    if (auto element = HTMLMediaElement::bestMediaElementForRemoteControls(MediaElementSession::PlaybackControlsPurpose::NowPlaying))
        return &element->mediaSession();

    return nullptr;
}

void MediaSessionManagerFbx::updateNowPlayingInfo()
{
    auto* platformSession = nowPlayingEligibleSession();
    if (!platformSession) {
        if (m_registeredAsNowPlayingApplication)
            m_nowPlayingManager->clearNowPlayingInfo();

        m_registeredAsNowPlayingApplication = false;
        m_nowPlayingActive = false;
        m_lastUpdatedNowPlayingTitle = emptyString();
        m_lastUpdatedNowPlayingDuration = NAN;
        m_lastUpdatedNowPlayingElapsedTime = NAN;
        m_lastUpdatedNowPlayingInfoUniqueIdentifier = { };
        return;
    }

    auto nowPlayingInfo = platformSession->nowPlayingInfo();
    if (!nowPlayingInfo)
        return;

    m_haveEverRegisteredAsNowPlayingApplication = true;

    if (m_nowPlayingManager->setNowPlayingInfo(*nowPlayingInfo))

    if (!m_registeredAsNowPlayingApplication) {
        m_registeredAsNowPlayingApplication = true;
        providePresentingApplicationPIDIfNecessary();
    }

    if (!nowPlayingInfo->title.isEmpty())
        m_lastUpdatedNowPlayingTitle = nowPlayingInfo->title;

    double duration = nowPlayingInfo->duration;
    if (std::isfinite(duration) && duration != MediaPlayer::invalidTime())
        m_lastUpdatedNowPlayingDuration = duration;

    m_lastUpdatedNowPlayingInfoUniqueIdentifier = nowPlayingInfo->uniqueIdentifier;

    double currentTime = nowPlayingInfo->currentTime;
    if (std::isfinite(currentTime) && currentTime != MediaPlayer::invalidTime() && nowPlayingInfo->supportsSeeking)
        m_lastUpdatedNowPlayingElapsedTime = currentTime;

    m_nowPlayingActive = nowPlayingInfo->allowsNowPlayingControlsVisibility;

    auto session = m_sessions.get(platformSession->mediaSessionIdentifier());
    session->updateNowPlaying(*nowPlayingInfo);
}

void MediaSessionManagerFbx::dispatch(PlatformMediaSession::RemoteControlCommandType platformCommand, PlatformMediaSession::RemoteCommandArgument argument)
{
    m_isSeeking = platformCommand == PlatformMediaSession::SeekToPlaybackPositionCommand;
    m_nowPlayingManager->didReceiveRemoteControlCommand(platformCommand, argument);
}

} // namespace WebCore

#endif //USE(GLIB) && ENABLE(MEDIA_SESSION) && USE(FBX_API)
