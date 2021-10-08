#include "config.h"
#include "MediaSessionFbx.h"

#if USE(GLIB) && ENABLE(MEDIA_SESSION) && USE(FBX_API)

#include "MediaSessionManagerFbx.h"

#include <mediacontroller.h>

#define WEBKIT_MEDIASESSION_FBXPULSE_MANAGER "WPEWebProcess"
#define WEBKIT_MEDIASESSION_FBXPULSE_STREAM  "*"

namespace WebCore {

std::unique_ptr<MediaSessionFbx> MediaSessionFbx::create(MediaSessionManagerFbx& manager, MediaSessionIdentifier identifier)
{
    return makeUnique<MediaSessionFbx>(manager, identifier);
}

MediaSessionFbx::MediaSessionFbx(MediaSessionManagerFbx& manager, MediaSessionIdentifier identifier)
    : m_identifier(identifier)
    , m_manager(manager)
{
    m_instanceId = makeString(identifier.toUInt64());
    mediacontroller_register_player(Fbxbus::singleton(),
                                        m_instanceId.ascii().data(),
                                        WEBKIT_MEDIASESSION_FBXPULSE_MANAGER,
                                        WEBKIT_MEDIASESSION_FBXPULSE_STREAM,
                                        MEDIACONTROLLER_PLAYER_CAPABILITIES_PLAY
                                        | MEDIACONTROLLER_PLAYER_CAPABILITIES_PAUSE
                                        | MEDIACONTROLLER_PLAYER_CAPABILITIES_STOP
                                        | MEDIACONTROLLER_PLAYER_CAPABILITIES_NEXT
                                        | MEDIACONTROLLER_PLAYER_CAPABILITIES_PREV
                                        | MEDIACONTROLLER_PLAYER_CAPABILITIES_SEEK_TO);
}

MediaSessionFbx::~MediaSessionFbx()
{
    mediacontroller_unregister_player(Fbxbus::singleton(), m_instanceId.ascii().data());
}

void MediaSessionFbx::emitPositionChanged(double time)
{
    int64_t position = time * 1000000;
    struct mediacontroller_player_state mediactl_state;
    mediactl_state.duration_ms = this->nowPlayingInfo()->duration*1000000;
    mediactl_state.position_ms = position;

    if (this->nowPlayingInfo()->isPlaying){
        mediactl_state.playback_state = MEDIACONTROLLER_PLAYBACK_STATE_PLAYING;
    } else {
        mediactl_state.playback_state = MEDIACONTROLLER_PLAYBACK_STATE_PAUSED;
    }

    mediacontroller_state_set(Fbxbus::singleton(),
                            m_instanceId.ascii().data(),
                            &mediactl_state);
}

void MediaSessionFbx::updateNowPlaying(NowPlayingInfo& nowPlayingInfo)
{
    UNUSED_PARAM(nowPlayingInfo);
}

void MediaSessionFbx::emitPropertiesChanged()
{
}

void MediaSessionFbx::playbackStatusChanged(PlatformMediaSession& platformSession)
{
    std::optional<const PlatformMediaSession*> session = &platformSession;
    auto state = [this, session = WTFMove(session)]() -> PlatformMediaSession::State {
        if (session)
            return session.value()->state();

        auto* nowPlayingSession = m_manager.nowPlayingEligibleSession();
        if (nowPlayingSession)
            return nowPlayingSession->state();

        return PlatformMediaSession::State::Idle;
    }();

    struct mediacontroller_player_state mediactl_state;
    mediactl_state.duration_ms = this->nowPlayingInfo()->duration*1000000;
    mediactl_state.position_ms = this->nowPlayingInfo()->currentTime*1000000;

    switch (state) {
    case PlatformMediaSession::State::Playing:
        mediactl_state.playback_state = MEDIACONTROLLER_PLAYBACK_STATE_PLAYING;
        break;
    case PlatformMediaSession::State::Paused:
        mediactl_state.playback_state = MEDIACONTROLLER_PLAYBACK_STATE_PAUSED;
        break;
    case PlatformMediaSession::State::Idle:
    case PlatformMediaSession::State::Interrupted:
    case PlatformMediaSession::State::Autoplaying:
        mediactl_state.playback_state = MEDIACONTROLLER_PLAYBACK_STATE_STOPPED;
        break;
    }

    mediacontroller_state_set(Fbxbus::singleton(),
                            m_instanceId.ascii().data(),
                            &mediactl_state);

    return;
}

std::optional<NowPlayingInfo> MediaSessionFbx::nowPlayingInfo()
{
    std::optional<NowPlayingInfo> nowPlayingInfo;
    m_manager.forEachMatchingSession([&](auto& session) {
        return session.mediaSessionIdentifier() == m_identifier;
    }, [&](auto& session) {
        nowPlayingInfo = session.nowPlayingInfo();
    });
    return nowPlayingInfo;
}

} // namespace WebCore

#endif // USE(GLIB) && ENABLE(MEDIA_SESSION) && USE(FBX_API)

