#ifndef AUDIO_MUSICPLAYER_HPP
#define AUDIO_MUSICPLAYER_HPP

#include <memory>
#include <string>

#include "Util/BGM.hpp"

// 背景音樂播放器：封裝 Util::BGM，記住目前曲目，只有在「換曲」時才重新載入並播放。
// 這樣在共用同一首 BGM 的選單畫面之間切換時，音樂不會被打斷、重頭播 (SRP)。
// App 持有一個，由各畫面 OnEnter 呼叫 Play 指定自己的曲目。
class MusicPlayer {
public:
    void Play(const std::string& path);  // 切到 path 並無限循環；已在播同一首則不動
    void SetVolume(int volume);          // 0..128

private:
    std::unique_ptr<Util::BGM> m_Bgm;    // 延遲建立 (GL/Mixer 就緒後第一次 Play 才建)
    std::string m_Current;               // 目前曲目路徑 (空 = 尚未播放)
    int m_Volume = 36;                   // 預設音量 (128 為最大)
};

#endif
