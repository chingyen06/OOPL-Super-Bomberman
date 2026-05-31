#include "Audio/SfxPlayer.hpp"

void SfxPlayer::Play(const std::string& path, int volume0to128) {
    if (!m_Sfx) {
        m_Sfx = std::make_unique<Util::SFX>(path);
        m_Current = path;
    } else if (path != m_Current) {
        m_Sfx->LoadMedia(path);
        m_Current = path;
    }
    m_Sfx->SetVolume(volume0to128 < 0 ? 0 : (volume0to128 > 128 ? 128 : volume0to128));
    m_Sfx->Play(0);  // loop = 0 → 播放一次
}
