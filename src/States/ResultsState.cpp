#include "States/ResultsState.hpp"

#include <string>

#include "Core/App.hpp"
#include "States/MainMenuState.hpp"
#include "States/MenuCommon.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"

void ResultsState::OnEnter(App& app) {
    app.PlayMusic(RESOURCE_DIR"/Sound/victory.mp3");  // 勝利/結算曲 (單一 BGM，會停掉戰鬥曲)
    auto& root = app.Root();
    const MatchResult& r = app.LastResult();
    const int total = app.Profile().Coins();
    const std::string earnedStr = "+" + std::to_string(r.coinsEarned);
    const std::string totalStr  = std::to_string(total);

    // 勝利圖是不透明全螢幕圖 (z=99)，疊加文字需 z>99。把數字填回原本被清空的欄位。
    auto add = [&](const std::string& s, float x, float y, float scale, Util::Color col) {
        auto t = std::make_shared<UIText>(s, x, y, 100.0f, col);
        t->SetScale(scale, scale);
        root.AddChild(t);
        m_Nodes.push_back(t);
    };

    // 左側「獲得寶石」明細：參加戰鬥 (+本場) / 總計
    add(earnedStr, 18.0f, -201.0f, 0.7f, GoldText());
    add(totalStr,  18.0f, -233.0f, 0.7f, DarkText());
    // 右側金幣面板：總額 (緊鄰品牌幣) + 本場 callout (+金幣，略往下對齊膠囊)
    add(totalStr,  280.0f, -201.0f, 0.85f, DarkText());
    add(earnedStr, 479.0f, -201.0f, 0.7f,  GoldText());
}

void ResultsState::OnExit(App& app) {
    auto& root = app.Root();
    for (auto& n : m_Nodes) root.RemoveChild(n);
    m_Nodes.clear();
    app.HideWinScreens();
    app.Session().Clear();
}

void ResultsState::OnUpdate(App& app) {
    if (Util::Input::IsKeyUp(Util::Keycode::SPACE)) {
        LOG_INFO("Return to Main Menu");
        app.TransitionTo(std::make_unique<MainMenuState>());
    }
}
