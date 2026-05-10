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
    void Update(int gameTimeTicks, const std::vector<std::shared_ptr<Player>>& players, const std::vector<bool>& chestStatus, Util::Renderer& root);
    void Clear(Util::Renderer& root);

private:
    std::shared_ptr<UIImage> m_TimerBackground;
    std::shared_ptr<UIImage> m_CrownImage;
    std::shared_ptr<UIText> m_TimerText;
    std::vector<std::shared_ptr<UIImage>> m_KeyIndicators;
    std::vector<std::shared_ptr<UIImage>> m_ChestPool;
    std::vector<bool> m_LastChestStatus; // �аO�֨�
    int m_LastSeconds = -1;
};

#endif