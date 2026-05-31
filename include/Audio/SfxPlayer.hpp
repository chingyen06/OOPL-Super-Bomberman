#ifndef AUDIO_SFXPLAYER_HPP
#define AUDIO_SFXPLAYER_HPP

#include <memory>
#include <string>

#include "Util/SFX.hpp"

// 一次性音效播放器：封裝 Util::SFX (短音效)，與 MusicPlayer (長 BGM) 對稱。
// 同一個檔案重複播放時沿用已載入的 chunk，僅在換檔時重新載入 (SRP)。
class SfxPlayer {
public:
    void Play(const std::string& path, int volume0to128);  // 以指定音量播放一次

private:
    std::unique_ptr<Util::SFX> m_Sfx;  // 延遲建立
    std::string m_Current;
};

#endif
