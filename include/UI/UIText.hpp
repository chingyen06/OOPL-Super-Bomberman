#ifndef UITEXT_HPP
#define UITEXT_HPP

#include "Util/GameObject.hpp"
#include "Util/Text.hpp"
#include "Util/Color.hpp"
#include <string>
#include <memory>

class UIText : public Util::GameObject {
public:
    UIText(const std::string& text, float x = 0.0f, float y = 0.0f, float z = 100.0f);
    void SetText(const std::string& text);
    void SetPosition(float x, float y);
private:
    std::shared_ptr<Util::Text> m_Text;
};

#endif
