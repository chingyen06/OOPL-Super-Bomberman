#include "UI/UIGroup.hpp"

void UIGroup::Add(Util::Renderer& root, const std::shared_ptr<Util::GameObject>& node) {
    if (!node) return;
    root.AddChild(node);
    m_Nodes.push_back(node);
}

void UIGroup::Track(const std::shared_ptr<Util::GameObject>& node) {
    if (!node) return;
    m_Nodes.push_back(node);
}

void UIGroup::Attach(Util::Renderer& root) {
    for (auto& n : m_Nodes) root.AddChild(n);
}

void UIGroup::Detach(Util::Renderer& root) {
    for (auto& n : m_Nodes) root.RemoveChild(n);
}

void UIGroup::Clear(Util::Renderer& root) {
    for (auto& n : m_Nodes) root.RemoveChild(n);
    m_Nodes.clear();
}
