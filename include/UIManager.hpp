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

class UIImage : public Util::GameObject {
public:
    UIImage(const std::string& imagePath, int x, int y);

    void SetPosition(float x, float y);
};

class UIText : public Util::GameObject {
public:
    UIText(const std::string& text, int x, int y);
    void SetText(const std::string& text);
private:
    std::shared_ptr<Util::Text> m_Text;
};

class UIManager {
public:
    void Init(Util::Renderer& root);
    void Update(int gameTimeTicks, const std::vector<std::shared_ptr<Player>>& players, Util::Renderer& root);
    void Clear(Util::Renderer& root);

private:
    std::shared_ptr<UIImage> m_TimerBackground;
    std::shared_ptr<UIImage> m_CrownImage;
    std::shared_ptr<UIText> m_TimerText;
    std::vector<std::shared_ptr<UIImage>> m_KeyIndicators;
};

#endif