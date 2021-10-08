#pragma once

#if USE(GLIB) && ENABLE(MEDIA_SESSION) && USE(FBX_API)

#include "MediaSessionIdentifier.h"
#include "PlatformMediaSession.h"

namespace WebCore {
class MediaSessionManagerFbx;

class MediaSessionFbx {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static std::unique_ptr<MediaSessionFbx> create(MediaSessionManagerFbx&, MediaSessionIdentifier);
    explicit MediaSessionFbx(MediaSessionManagerFbx&, MediaSessionIdentifier);
    ~MediaSessionFbx();
    MediaSessionManagerFbx& manager() const { return m_manager; }

    void emitPositionChanged(double time);
    void updateNowPlaying(NowPlayingInfo&);
    void playbackStatusChanged(PlatformMediaSession&);
    String m_instanceId;
    MediaSessionIdentifier m_identifier;

private:
    void emitPropertiesChanged();
    std::optional<NowPlayingInfo> nowPlayingInfo();
    MediaSessionManagerFbx& m_manager;
};

} // namespace WebCore

#endif //#if USE(GLIB) && ENABLE(MEDIA_SESSION) && USE(FBX_API)
