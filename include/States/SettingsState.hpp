#ifndef SETTINGSSTATE_HPP
#define SETTINGSSTATE_HPP

#include <memory>
#include <string>
#include <vector>

#include "Config/KeyBindings.hpp"
#include "States/IGameState.hpp"
#include "States/MenuCommon.hpp"
#include "UI/Slider.hpp"
#include "UI/UIImage.hpp"
#include "UI/UIText.hpp"
#include "Util/Renderer.hpp"

// 設定 (操作表；可改鍵)：方向鍵選格、空白鍵重設該鍵 (再按一下新鍵)、Del 清除 (留空)、X 返回。
class SettingsState : public IGameState {
public:
    void OnEnter(App& app) override;
    void OnExit(App& app) override;
    void OnUpdate(App& app) override;
    bool WantsCursor(App& /*app*/) override { return true; }  // 允許滑鼠：點格選取 / 改鍵

private:
    static constexpr int kActions = KeyBindings::kActions;  // 操作列數 (上/下/左/右/放炸彈/武器)
    static constexpr int kBgmRow  = kActions;               // 「背景音樂」音量列 (排在操作列之後)
    static constexpr int kRows    = kActions + 1;           // 可上下導覽的總列數 (操作 + 背景音樂)
    int  m_Row = 0;             // 0..kActions-1 = 操作列；kBgmRow = 背景音樂音量列
    int  m_Col = 0;             // 0 = 玩家1，1 = 玩家2
    bool m_Awaiting = false;    // 等待輸入新鍵中
    bool m_IgnoreConfirm = false;  // 綁定空白/Enter 後，吞掉其放開避免又觸發「重設」

    void Rebuild(App& app);
    void ClearTable(Util::Renderer& root);
    void Build(App& app);
    void AddStrip(Util::Renderer& root, const std::string& img, float y, float w, float z);
    void AddText(Util::Renderer& root, const std::string& t, float x, float y, Util::Color c);
    void AddKeyBox(Util::Renderer& root, const std::string& key, float cx, float y, bool selected);

    std::shared_ptr<UIImage> m_Gear;
    std::shared_ptr<UIText>  m_Title;
    std::vector<std::shared_ptr<UIImage>> m_Imgs;
    std::vector<std::shared_ptr<UIText>>  m_Txts;
    Slider  m_BgmSlider;   // 背景音樂音量 (分段式、可滑鼠拖曳)
    KeyHint m_Hint;
};

#endif
