#ifndef MENU_HPP
#define MENU_HPP

#include <vector>
#include <string>
#include <functional>
#include <memory>
#include "UIManager.hpp"
#include "Util/Renderer.hpp"

class Menu {
public:
    void AddOption(const std::string& text, std::function<void()> onSelect);

    void Show(Util::Renderer& root, float startX, float startY);
    void Hide(Util::Renderer& root);
    void Update();

    bool IsVisible() const { return m_IsVisible; }

private:
    int m_SelectedIndex = 0;
    bool m_IsVisible = false;

    std::vector<std::string> m_RawTexts;
    std::vector<std::shared_ptr<UIText>> m_OptionTexts;
    std::vector<std::function<void()>> m_Callbacks;

    void UpdateCursor();
};

#endif