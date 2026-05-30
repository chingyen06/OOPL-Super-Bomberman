#ifndef UITEXT_HPP
#define UITEXT_HPP

#include "Util/GameObject.hpp"
#include "Util/Text.hpp"
#include "Util/Color.hpp"
#include <string>
#include <memory>

class UIText : public Util::GameObject {
public:
    UIText(const std::string& text, float x = 0.0f, float y = 0.0f, float z = 100.0f,
           const Util::Color& color = Util::Color::FromName(Util::Colors::BLACK));
    void SetText(const std::string& text);
    void SetPosition(float x, float y);
    void SetScale(float sx, float sy);
    void SetColor(const Util::Color& color);
    float GetWidth() const;   // 目前文字的原始像素寬度 (未套用 transform 縮放)
private:
    std::shared_ptr<Util::Text> m_Text;
};

#endif
