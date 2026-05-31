#include "Audio/MusicPlayer.hpp"

void MusicPlayer::Play(const std::string& path) {
    if (m_Bgm && path == m_Current) return;  // 已在播同一首 → 不打斷、不重頭播

    m_Current = path;
    if (!m_Bgm) {
        m_Bgm = std::make_unique<Util::BGM>(path);
    } else {
        m_Bgm->LoadMedia(path);
    }
    m_Bgm->SetVolume(m_Volume);
    m_Bgm->Play(-1);  // 無限循環
}

void MusicPlayer::SetVolume(int volume) {
    m_Volume = volume;
    if (m_Bgm) m_Bgm->SetVolume(volume);
}
