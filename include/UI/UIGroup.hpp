#ifndef UIGROUP_HPP
#define UIGROUP_HPP

#include <memory>
#include <vector>

#include "Util/GameObject.hpp"
#include "Util/Renderer.hpp"

// 一組「共同進出場景」的 UI 物件：負責整批掛上 / 移除的記帳。
// 取代各畫面與元件各自重抄的 vector<shared_ptr<UIx>> + 逐一 AddChild / RemoveChild 樣板。
//   - 傳統 OOP：把重複的集合管理行為封裝成一個可重用類別 (消除重複設計)。
//   - 析構不自動卸載 (本類不持有 root)；拆除請明確呼叫 Detach / Clear，
//     因此可安全地被值複製 / 回傳 (KeyHint / CoinHud 即以值回傳)。
class UIGroup {
public:
    // 建立後立刻掛到 root 並記帳 (最常見用法：邊建邊掛)。
    void Add(Util::Renderer& root, const std::shared_ptr<Util::GameObject>& node);

    // 只記帳、不掛上 (供「先預載、稍後再 Attach」的情境，如暫停面板裝飾)。
    void Track(const std::shared_ptr<Util::GameObject>& node);

    // 把所有已記帳物件掛上 / 從 root 移除 (保留記帳，可重複進出場景)。
    void Attach(Util::Renderer& root);
    void Detach(Util::Renderer& root);

    // 從 root 移除全部並清空記帳 (徹底拆除)。
    void Clear(Util::Renderer& root);

    bool Empty() const { return m_Nodes.empty(); }

private:
    std::vector<std::shared_ptr<Util::GameObject>> m_Nodes;
};

#endif
