#ifndef UIMANAGER_HPP
#define UIMANAGER_HPP

#include <vector>
#include <memory>
#include <string>
#include "Util/GameObject.hpp"
#include "Util/Text.hpp"
#include "Util/Color.hpp"
#include "Util/Image.hpp"
#include "Player.hpp"
#include "Util/Renderer.hpp"
#include "UI/UIImage.hpp"
#include "UI/UIText.hpp"

class UIManager {
public:
    void Init(Util::Renderer& root, int totalChests);
    void Update(int gameTimeTicks, const std::vector<std::shared_ptr<Player>>& players, const std::vector<bool>& chestStatus, Util::Renderer& root, bool cheatEnabled);
    void Clear(Util::Renderer& root);

private:
    std::shared_ptr<UIImage> m_TimerBackground;
    std::shared_ptr<UIImage> m_CrownImage;
    std::shared_ptr<UIText> m_TimerText;
    std::shared_ptr<UIText> m_CheatText;  // 作弊模式啟用時的螢幕提示
    bool m_LastCheatEnabled = false;      // diff 用，避免每幀重設文字
    std::vector<std::shared_ptr<UIImage>> m_KeyIndicators;
    std::vector<std::shared_ptr<UIImage>> m_StunIndicators;  // 暈眩星星 (浮在被擊暈玩家頭上、旋轉)
    int m_StunSpinTick = 0;                                  // 星星旋轉計時
    std::vector<std::shared_ptr<UIImage>> m_ChestPool;
    std::shared_ptr<Util::Image> m_ChestOpenedImage;  // 預載：寶箱開啟圖，避免每次開啟時讀檔
    std::vector<bool> m_LastChestStatus; // Cached marker for diffing chest state
    int m_LastSeconds = -1;
};

#endif