#include "App.hpp"

#include <memory>
#include <string>
#include <vector>

#include "UI/UIButtonList.hpp"
#include "UI/UIText.hpp"
#include "Util/Color.hpp"
#include "Util/Image.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"

// ----------------------------- State forward decls -----------------------------

class TitleScreenState;
class MainMenuState;
class BattleSetupState;
class RulesState;
class TeamSelectState;
class LevelSelectState;
class GameplayState;
class GamePausedState;
class GameEndState;

// 共用：選單字色 (淺色背景用深字)
static Util::Color DarkText()   { return Util::Color::FromName(Util::Colors::BLACK); }
static Util::Color WhiteText()  { return Util::Color::FromName(Util::Colors::WHITE); }
static Util::Color YellowText() { return Util::Color::FromName(Util::Colors::YELLOW); }

// 「更換規則」可選的回合時間 (秒 / 顯示文字)
static const int   kTimeSecondsOptions[] = { 60, 120, 180, 300 };
static const char* kTimeLabels[]         = { "1 分", "2 分", "3 分", "5 分" };
static constexpr int kTimeOptionCount    = 4;

// 文字貼圖右側有留白 → 往右補正才水平置中；垂直只需微調 (量測值)。
static constexpr float kLabelXNudge = 7.0f;
static constexpr float kLabelYNudge = 1.0f;

// 共用：在畫面底部建立按鍵提示文字並掛上 root，回傳以便 OnExit 移除
static std::shared_ptr<UIText> AddHint(App& app, const std::string& text) {
    auto hint = std::make_shared<UIText>(text, 0.0f, -330.0f, 30.0f,
                                         Util::Color::FromName(Util::Colors::DIM_GRAY));
    app.Root().AddChild(hint);
    return hint;
}

// ----------------------------- 封面 -----------------------------

class TitleScreenState : public IGameState {
public:
    void OnEnter(App& app) override { app.ShowCover(); }
    void OnExit (App& app) override { app.HideCover(); }
    void OnUpdate(App& app) override;
};

// ----------------------------- 主選單 -----------------------------

class MainMenuState : public IGameState {
public:
    void OnEnter(App& app) override {
        app.ShowMenuBg();
        auto& root = app.Root();

        // 大標題 (置中靠上)
        m_Title = std::make_shared<UIText>("SUPER BOMBERMAN", 0.0f, 250.0f, 30.0f, DarkText());
        root.AddChild(m_Title);
        m_Subtitle = std::make_shared<UIText>("城堡模式", 0.0f, 195.0f, 30.0f,
                                              Util::Color::FromName(Util::Colors::DIM_GRAY));
        root.AddChild(m_Subtitle);

        // 功能選單 (只保留真正能用的項目)
        m_Menu.Init(RESOURCE_DIR"/Image/btn.png", RESOURCE_DIR"/Image/btn_sel.png");
        m_Menu.AddItem("開始對戰");
        m_Menu.AddItem("離開遊戲");
        m_Menu.Show(root, 0.0f, 40.0f, 0.0f, -100.0f);

        m_Hint = AddHint(app, "方向鍵：選擇　　空白鍵：確定　　ESC：離開");
    }
    void OnExit(App& app) override {
        auto& root = app.Root();
        m_Menu.Hide(root);
        root.RemoveChild(m_Title);
        root.RemoveChild(m_Subtitle);
        root.RemoveChild(m_Hint);
        app.HideMenuBg();
    }
    void OnUpdate(App& app) override;  // 見下方

private:
    UIButtonList m_Menu;
    std::shared_ptr<UIText> m_Title;
    std::shared_ptr<UIText> m_Subtitle;
    std::shared_ptr<UIText> m_Hint;
};

// ----------------------------- 對戰設定 (離線戰鬥) -----------------------------

class BattleSetupState : public IGameState {
public:
    void OnEnter(App& app) override {
        app.ShowMenuBg();
        auto& root = app.Root();

        m_Title = std::make_shared<UIText>("離線戰鬥", -505.0f, 320.0f, 30.0f, DarkText());
        root.AddChild(m_Title);

        // 左側導覽
        m_Nav.Init(RESOURCE_DIR"/Image/btn.png", RESOURCE_DIR"/Image/btn_sel.png");
        m_Nav.AddItem("對戰開始");
        m_Nav.AddItem("更換規則");
        m_Nav.AddItem("選擇隊伍");
        m_Nav.AddItem("選擇關卡");
        m_Nav.Show(root, -470.0f, 170.0f, 0.0f, -80.0f);

        // 成員格 8 格：card0 = 玩家1 (守)；card1..7 = 進攻席位 slot 0..6，反映「選擇隊伍」設定。
        const MatchConfig& cfg = app.Session().Config();
        const float colXs[4] = { -87.0f, 91.0f, 269.0f, 447.0f };
        const float rowYs[2] = { 120.0f, -10.0f };
        for (int i = 0; i < 8; i++) {
            const float x = colXs[i % 4];
            const float y = rowYs[i / 4];
            auto slot = std::make_shared<UIImage>(RESOURCE_DIR"/Image/slot.png", x, y, 20.0f);

            std::string name;
            Util::Color color = DarkText();
            if (i == 0) {
                name = "玩家 1";
            }
            else {
                const int s = i - 1;  // 進攻席位
                const MatchConfig::SlotMode m = cfg.AttackerSlot(s);
                if (m == MatchConfig::SlotMode::Off) {
                    name = "—";
                    color = Util::Color::FromName(Util::Colors::GRAY);  // 未加入 → 灰
                }
                else if (m == MatchConfig::SlotMode::Human) {
                    name = "玩家 2";
                }
                else {
                    name = "電腦";  // AI 進攻方 (含玩家 2 席位由 AI 補上的情況)
                }
            }
            auto label = std::make_shared<UIText>(name, x + kLabelXNudge, y - kLabelYNudge, 30.0f, color);
            root.AddChild(slot);
            root.AddChild(label);
            m_Slots.push_back(slot);
            m_SlotLabels.push_back(label);
        }

        m_LevelInfo = std::make_shared<UIText>("目前關卡：" + std::to_string(app.SelectedLevel()),
                                               180.0f, -130.0f, 30.0f, DarkText());
        root.AddChild(m_LevelInfo);

        m_Hint = AddHint(app, "方向鍵：選擇　　空白鍵：確定　　X：返回");
    }
    void OnExit(App& app) override {
        auto& root = app.Root();
        m_Nav.Hide(root);
        root.RemoveChild(m_Title);
        root.RemoveChild(m_LevelInfo);
        root.RemoveChild(m_Hint);
        for (auto& s : m_Slots)      root.RemoveChild(s);
        for (auto& l : m_SlotLabels) root.RemoveChild(l);
        app.HideMenuBg();
    }
    void OnUpdate(App& app) override;

private:
    UIButtonList m_Nav;
    std::shared_ptr<UIText> m_Title;
    std::shared_ptr<UIText> m_LevelInfo;
    std::shared_ptr<UIText> m_Hint;
    std::vector<std::shared_ptr<UIImage>> m_Slots;
    std::vector<std::shared_ptr<UIText>>  m_SlotLabels;
};

// ----------------------------- 更換規則 (唯讀) -----------------------------

// 可調整的規則：上下選擇、左右調整 (時間 / 源石精靈 / 砲台)，直接寫入 GameSession 的 MatchConfig。
class RulesState : public IGameState {
public:
    void OnEnter(App& app) override {
        app.ShowMenuBg();
        auto& root = app.Root();

        m_RowNormal = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/row_grey.png");
        m_RowSel    = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/row_sel.png");
        m_BtnNormal = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/btn.png");
        m_BtnSel    = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/btn_sel.png");

        m_Title = std::make_shared<UIText>("更換規則", -505.0f, 320.0f, 30.0f, DarkText());
        root.AddChild(m_Title);

        // 固定顯示列 (不可選)
        m_FixedRow = std::make_shared<UIImage>(RESOURCE_DIR"/Image/row_grey.png", 0.0f, 200.0f, 20.0f);
        m_FixedLabel = std::make_shared<UIText>("對戰模式：城堡", kLabelXNudge, 200.0f - kLabelYNudge, 30.0f, WhiteText());
        root.AddChild(m_FixedRow);
        root.AddChild(m_FixedLabel);

        // 3 個可調列
        const float ys[kRows] = { 130.0f, 70.0f, 10.0f };
        for (int i = 0; i < kRows; i++) {
            m_Rows[i] = std::make_shared<UIImage>(0.0f, ys[i], 20.0f);
            m_Rows[i]->SetDrawable(m_RowNormal);
            // 佔位字串 (非空) — 立即由 Refresh() 覆寫；空字串會讓 TTF 產生 null surface 而崩潰
            m_RowLabels[i] = std::make_shared<UIText>("-", kLabelXNudge, ys[i] - kLabelYNudge, 30.0f, WhiteText());
            root.AddChild(m_Rows[i]);
            root.AddChild(m_RowLabels[i]);
        }

        // 確定鈕
        m_DoneBtn = std::make_shared<UIImage>(0.0f, -90.0f, 20.0f);
        m_DoneBtn->SetDrawable(m_BtnNormal);
        m_DoneLabel = std::make_shared<UIText>("確定", kLabelXNudge, -90.0f - kLabelYNudge, 30.0f, DarkText());
        root.AddChild(m_DoneBtn);
        root.AddChild(m_DoneLabel);

        m_Sel = 0;
        Refresh(app);
        m_Hint = AddHint(app, "上下：選擇　左右：調整　　空白鍵：確定　　X：返回");
    }
    void OnExit(App& app) override {
        auto& root = app.Root();
        root.RemoveChild(m_Title);
        root.RemoveChild(m_Hint);
        root.RemoveChild(m_FixedRow);
        root.RemoveChild(m_FixedLabel);
        root.RemoveChild(m_DoneBtn);
        root.RemoveChild(m_DoneLabel);
        for (int i = 0; i < kRows; i++) {
            root.RemoveChild(m_Rows[i]);
            root.RemoveChild(m_RowLabels[i]);
        }
        app.HideMenuBg();
    }
    void OnUpdate(App& app) override;

private:
    static constexpr int kRows  = 3;  // 時間 / 源石精靈 / 砲台
    static constexpr int kItems = 4;  // 上面 3 列 + 確定

    int TimeIndex(int seconds) const {
        for (int i = 0; i < kTimeOptionCount; i++) if (kTimeSecondsOptions[i] == seconds) return i;
        return 2;  // 預設 3 分
    }
    void Adjust(MatchConfig& cfg, int dir) {
        if (m_Sel == 0) {
            const int ti = (TimeIndex(cfg.RoundSeconds()) + dir + kTimeOptionCount) % kTimeOptionCount;
            cfg.SetRoundSeconds(kTimeSecondsOptions[ti]);
        }
        else if (m_Sel == 1) cfg.SetSpiritsEnabled(!cfg.SpiritsEnabled());
        else if (m_Sel == 2) cfg.SetTurretsEnabled(!cfg.TurretsEnabled());
    }
    void Refresh(App& app) {
        const MatchConfig& cfg = app.Session().Config();
        m_RowLabels[0]->SetText(std::string("時間：")    + kTimeLabels[TimeIndex(cfg.RoundSeconds())]);
        m_RowLabels[1]->SetText(std::string("源石精靈：") + (cfg.SpiritsEnabled() ? "開" : "關"));
        m_RowLabels[2]->SetText(std::string("砲台：")    + (cfg.TurretsEnabled() ? "開" : "關"));
        for (int i = 0; i < kRows; i++) m_Rows[i]->SetDrawable(m_Sel == i ? m_RowSel : m_RowNormal);
        const bool doneSel = (m_Sel == kRows);
        m_DoneBtn->SetDrawable(doneSel ? m_BtnSel : m_BtnNormal);
        m_DoneLabel->SetColor(doneSel ? WhiteText() : DarkText());
    }

    int m_Sel = 0;
    std::shared_ptr<UIText>  m_Title, m_Hint, m_FixedLabel, m_DoneLabel;
    std::shared_ptr<UIImage> m_FixedRow, m_DoneBtn;
    std::shared_ptr<Util::Image> m_RowNormal, m_RowSel, m_BtnNormal, m_BtnSel;
    std::shared_ptr<UIImage> m_Rows[kRows];
    std::shared_ptr<UIText>  m_RowLabels[kRows];
};

// ----------------------------- 選擇隊伍 -----------------------------

// 玩家 1 永遠是唯一守方。下方是進攻方名單 (玩家2 + 電腦)，每一席可調：
//   - 玩家 2 (slot 0)：不加入 / 電腦 / 人類 (人類用方向鍵 + 右 Shift)
//   - 電腦 (slot 1..7)：攻擊方 / 不加入
// 上下選擇席位、左右調整；至少 1 名攻擊方、最多 kMaxAttackers 名 (8)。
class TeamSelectState : public IGameState {
public:
    void OnEnter(App& app) override {
        app.ShowMenuBg();
        auto& root = app.Root();

        m_RowDef    = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/row_def.png");
        m_RowAtk    = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/row_atk.png");
        m_RowGrey   = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/row_grey.png");
        m_BtnNormal = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/btn.png");
        m_BtnSel    = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/btn_sel.png");

        m_Title = std::make_shared<UIText>("選擇隊伍", -505.0f, 320.0f, 30.0f, DarkText());
        root.AddChild(m_Title);

        for (int i = 0; i < kRows; i++) {  // row 0 = 玩家1；row 1..8 = 進攻席位 (slot 0..7)
            const float y = 230.0f - i * 48.0f;
            m_Rows[i] = std::make_shared<UIImage>(0.0f, y, 20.0f);
            m_Rows[i]->SetDrawable(m_RowGrey);
            m_RowLabels[i] = std::make_shared<UIText>("-", kLabelXNudge, y - kLabelYNudge, 30.0f, WhiteText());
            root.AddChild(m_Rows[i]);
            root.AddChild(m_RowLabels[i]);
        }

        m_DoneBtn = std::make_shared<UIImage>(0.0f, -215.0f, 20.0f);
        m_DoneBtn->SetDrawable(m_BtnNormal);
        m_DoneLabel = std::make_shared<UIText>("確定", kLabelXNudge, -215.0f - kLabelYNudge, 30.0f, DarkText());
        root.AddChild(m_DoneBtn);
        root.AddChild(m_DoneLabel);

        m_Sel = 0;  // 0..kAttackers-1 = 進攻席位；kDoneIndex = 確定
        Refresh(app);
        m_Hint = AddHint(app, "上下：選擇　左右：調整　　空白鍵：確定　　X：返回");
    }
    void OnExit(App& app) override {
        auto& root = app.Root();
        root.RemoveChild(m_Title);
        root.RemoveChild(m_Hint);
        root.RemoveChild(m_DoneBtn);
        root.RemoveChild(m_DoneLabel);
        for (int i = 0; i < kRows; i++) {
            root.RemoveChild(m_Rows[i]);
            root.RemoveChild(m_RowLabels[i]);
        }
        app.HideMenuBg();
    }
    void OnUpdate(App& app) override;

private:
    static constexpr int kAttackers = MatchConfig::kMaxAttackers;  // 8 個進攻席位
    static constexpr int kRows      = kAttackers + 1;              // 玩家1 + 8 席位 = 9 列
    static constexpr int kItems     = kAttackers + 1;              // 8 席位 + 確定
    static constexpr int kDoneIndex = kAttackers;                  // 確定的 m_Sel 值

    void Adjust(MatchConfig& cfg, int dir) {
        if (m_Sel < 0 || m_Sel >= kAttackers) return;  // 只有席位列可調
        const MatchConfig::SlotMode cur = cfg.AttackerSlot(m_Sel);
        MatchConfig::SlotMode next;
        if (m_Sel == 0) {  // 玩家 2：循環 不加入 → 電腦 → 人類
            next = static_cast<MatchConfig::SlotMode>((static_cast<int>(cur) + dir + 3) % 3);
        }
        else {  // 電腦：切換 不加入 / 攻擊方
            next = (cur == MatchConfig::SlotMode::Off) ? MatchConfig::SlotMode::Computer
                                                       : MatchConfig::SlotMode::Off;
        }
        cfg.SetAttackerSlot(m_Sel, next);
        if (cfg.AttackerTotal() == 0) cfg.SetAttackerSlot(m_Sel, cur);  // 至少 1 攻擊方 → 還原
    }
    void Refresh(App& app) {
        const MatchConfig& cfg = app.Session().Config();

        // row 0：玩家 1 (固定守方)
        m_Rows[0]->SetDrawable(m_RowDef);
        m_RowLabels[0]->SetText("玩家 1　防守方");
        m_RowLabels[0]->SetColor(WhiteText());

        // row 1..8：進攻席位
        for (int slot = 0; slot < kAttackers; slot++) {
            const int r = slot + 1;
            const MatchConfig::SlotMode m = cfg.AttackerSlot(slot);
            const std::string name = (slot == 0) ? "玩家 2" : ("電腦 " + std::to_string(slot));
            const char* val = (m == MatchConfig::SlotMode::Off)   ? "不加入"
                            : (m == MatchConfig::SlotMode::Human)  ? "人類"
                            : (slot == 0)                          ? "電腦" : "攻擊方";
            m_Rows[r]->SetDrawable(m == MatchConfig::SlotMode::Off ? m_RowGrey : m_RowAtk);
            m_RowLabels[r]->SetText(name + "　" + val);
            m_RowLabels[r]->SetColor(m_Sel == slot ? YellowText() : WhiteText());  // 選取席位→黃字
        }

        const bool doneSel = (m_Sel == kDoneIndex);
        m_DoneBtn->SetDrawable(doneSel ? m_BtnSel : m_BtnNormal);
        m_DoneLabel->SetColor(doneSel ? WhiteText() : DarkText());
    }

    int m_Sel = 0;
    std::shared_ptr<UIText>  m_Title, m_Hint, m_DoneLabel;
    std::shared_ptr<UIImage> m_DoneBtn;
    std::shared_ptr<Util::Image> m_RowDef, m_RowAtk, m_RowGrey, m_BtnNormal, m_BtnSel;
    std::shared_ptr<UIImage> m_Rows[kRows];
    std::shared_ptr<UIText>  m_RowLabels[kRows];
};

// ----------------------------- 選擇關卡 -----------------------------

class LevelSelectState : public IGameState {
public:
    void OnEnter(App& app) override {
        app.ShowMenuBg();
        auto& root = app.Root();

        m_Title = std::make_shared<UIText>("選擇關卡", -505.0f, 320.0f, 30.0f, DarkText());
        root.AddChild(m_Title);

        m_Thumbs.Init(RESOURCE_DIR"/Image/thumb.png", RESOURCE_DIR"/Image/thumb_sel.png");
        for (int i = 1; i <= App::NumLevels(); i++) {
            m_Thumbs.AddItem("關卡 " + std::to_string(i));
        }
        m_Thumbs.Show(root, -300.0f, 30.0f, 300.0f, 0.0f);
        m_Thumbs.SetSelected(app.SelectedLevel() - 1);  // 對齊目前選擇

        m_Hint = AddHint(app, "方向鍵：選擇　　空白鍵：確定　　X：返回");
    }
    void OnExit(App& app) override {
        m_Thumbs.Hide(app.Root());
        app.Root().RemoveChild(m_Title);
        app.Root().RemoveChild(m_Hint);
        app.HideMenuBg();
    }
    void OnUpdate(App& app) override;

private:
    UIButtonList m_Thumbs;
    std::shared_ptr<UIText> m_Title;
    std::shared_ptr<UIText> m_Hint;
};

// ----------------------------- 遊戲 / 暫停 / 結束 -----------------------------

class GameplayState : public IGameState {
public:
    void OnUpdate(App& app) override;
};

class GamePausedState : public IGameState {
public:
    void OnEnter(App& app) override { app.ShowPauseMenu(); }
    void OnExit (App& app) override { app.HidePauseMenu(); }
    void OnUpdate(App& app) override {
        // ENTER / X = 返回遊戲；SPACE = 確認，由選單自己處理。
        if (Util::Input::IsKeyUp(Util::Keycode::RETURN) ||
            Util::Input::IsKeyUp(Util::Keycode::KP_ENTER) ||
            Util::Input::IsKeyUp(Util::Keycode::X)) {
            app.ResumeGame();
            return;
        }
        app.UpdatePauseMenu();
    }
};

class GameEndState : public IGameState {
public:
    void OnEnter(App& app) override { app.Session().Clear(); }
    void OnExit (App& app) override { app.HideWinScreens(); }
    void OnUpdate(App& app) override {
        if (Util::Input::IsKeyUp(Util::Keycode::SPACE)) {
            LOG_INFO("Return to Title Screen");
            app.TransitionTo(std::make_unique<TitleScreenState>());
        }
    }
};

// ----------------------------- State OnUpdate 實作 -----------------------------

void TitleScreenState::OnUpdate(App& app) {
    if (Util::Input::IsKeyUp(Util::Keycode::SPACE)) {
        LOG_INFO("Enter Main Menu");
        app.TransitionTo(std::make_unique<MainMenuState>());
    }
}

void MainMenuState::OnUpdate(App& app) {
    const int confirmed = m_Menu.Update();
    if (confirmed == 0) { app.TransitionTo(std::make_unique<BattleSetupState>()); return; }  // 開始對戰
    if (confirmed == 1) { app.RequestQuit(); return; }  // 離開遊戲 (ESC 亦可離開，由 App::Update 全域處理)
}

void BattleSetupState::OnUpdate(App& app) {
    const int confirmed = m_Nav.Update();
    switch (confirmed) {
        case 0: app.StartMatch(); return;                                        // 對戰開始
        case 1: app.TransitionTo(std::make_unique<RulesState>());      return;   // 更換規則
        case 2: app.TransitionTo(std::make_unique<TeamSelectState>()); return;   // 選擇隊伍
        case 3: app.TransitionTo(std::make_unique<LevelSelectState>());return;   // 選擇關卡
        default: break;
    }
    if (Util::Input::IsKeyUp(Util::Keycode::X)) {  // X = 返回
        app.TransitionTo(std::make_unique<MainMenuState>());
    }
}

void RulesState::OnUpdate(App& app) {
    MatchConfig& cfg = app.Session().Config();
    bool dirty = false;
    if (Util::Input::IsKeyUp(Util::Keycode::UP) || Util::Input::IsKeyUp(Util::Keycode::W)) {
        m_Sel = (m_Sel + kItems - 1) % kItems; dirty = true;
    }
    else if (Util::Input::IsKeyUp(Util::Keycode::DOWN) || Util::Input::IsKeyUp(Util::Keycode::S)) {
        m_Sel = (m_Sel + 1) % kItems; dirty = true;
    }
    else if (Util::Input::IsKeyUp(Util::Keycode::LEFT) || Util::Input::IsKeyUp(Util::Keycode::A)) {
        Adjust(cfg, -1); dirty = true;
    }
    else if (Util::Input::IsKeyUp(Util::Keycode::RIGHT) || Util::Input::IsKeyUp(Util::Keycode::D)) {
        Adjust(cfg, +1); dirty = true;
    }
    if (dirty) Refresh(app);

    const bool confirm = (m_Sel == kRows) && Util::Input::IsKeyUp(Util::Keycode::SPACE);  // 確定
    if (confirm || Util::Input::IsKeyUp(Util::Keycode::X)) {  // X = 返回
        app.TransitionTo(std::make_unique<BattleSetupState>());
    }
}

void TeamSelectState::OnUpdate(App& app) {
    MatchConfig& cfg = app.Session().Config();
    bool dirty = false;
    if (Util::Input::IsKeyUp(Util::Keycode::UP) || Util::Input::IsKeyUp(Util::Keycode::W)) {
        m_Sel = (m_Sel + kItems - 1) % kItems; dirty = true;
    }
    else if (Util::Input::IsKeyUp(Util::Keycode::DOWN) || Util::Input::IsKeyUp(Util::Keycode::S)) {
        m_Sel = (m_Sel + 1) % kItems; dirty = true;
    }
    else if (Util::Input::IsKeyUp(Util::Keycode::LEFT) || Util::Input::IsKeyUp(Util::Keycode::A)) {
        Adjust(cfg, -1); dirty = true;
    }
    else if (Util::Input::IsKeyUp(Util::Keycode::RIGHT) || Util::Input::IsKeyUp(Util::Keycode::D)) {
        Adjust(cfg, +1); dirty = true;
    }
    if (dirty) Refresh(app);

    const bool confirm = (m_Sel == kDoneIndex) && Util::Input::IsKeyUp(Util::Keycode::SPACE);  // 確定
    if (confirm || Util::Input::IsKeyUp(Util::Keycode::X)) {  // X = 返回
        app.TransitionTo(std::make_unique<BattleSetupState>());
    }
}

void LevelSelectState::OnUpdate(App& app) {
    const int picked = m_Thumbs.Update();
    if (picked >= 0) {
        app.SetSelectedLevel(picked + 1);
        app.TransitionTo(std::make_unique<BattleSetupState>());
        return;
    }
    if (Util::Input::IsKeyUp(Util::Keycode::X)) {  // X = 返回
        app.TransitionTo(std::make_unique<BattleSetupState>());
    }
}

void GameplayState::OnUpdate(App& app) {
    // ENTER 進入暫停 (凍結這一幀之後的所有 gameplay 更新)
    if (Util::Input::IsKeyUp(Util::Keycode::RETURN) || Util::Input::IsKeyUp(Util::Keycode::KP_ENTER)) {
        app.PauseGame();
        return;
    }

    auto& session = app.Session();
    session.Update();

    if (session.IsAttackerWin()) {
        LOG_INFO("Attacker Wins!");
        app.ShowAttackerWin();
        app.TransitionTo(std::make_unique<GameEndState>());
        return;
    }
    if (session.IsTimeUp()) {
        LOG_INFO("Defender Wins!");
        app.ShowDefenderWin();
        app.TransitionTo(std::make_unique<GameEndState>());
    }
}

// ----------------------------- App -----------------------------

void App::TransitionTo(std::unique_ptr<IGameState> next) {
    if (m_CurrentGameState) m_CurrentGameState->OnExit(*this);
    m_CurrentGameState = std::move(next);
    if (m_CurrentGameState) m_CurrentGameState->OnEnter(*this);
}

void App::Start() {
    LOG_TRACE("Start");

    m_CoverImage   = std::make_shared<UIImage>(RESOURCE_DIR"/Image/cover.jpg");        m_CoverImage->SetFullScreen();
    m_DefenseImage = std::make_shared<UIImage>(RESOURCE_DIR"/Image/defense_win.png");  m_DefenseImage->SetFullScreen();
    m_AttackImage  = std::make_shared<UIImage>(RESOURCE_DIR"/Image/attack_win.png");   m_AttackImage->SetFullScreen();
    m_MenuBg       = std::make_shared<UIImage>(RESOURCE_DIR"/Image/ui_bg.png", 0.0f, 0.0f, 0.0f); m_MenuBg->SetFullScreen();

    // 暫停選單 (順序需與 kCheatOptionIndex 對應)
    m_PauseMenu.Init();
    m_PauseMenu.AddOption("繼續",        [this]() { ResumeGame(); });          // index 0
    m_PauseMenu.AddOption("作弊模式：關", [this]() {                            // index 1 == kCheatOptionIndex
        m_Session.ToggleCheat();
        m_PauseMenu.SetOptionLabel(kCheatOptionIndex,
            m_Session.IsCheatEnabled() ? "作弊模式：開" : "作弊模式：關");
    });
    m_PauseMenu.AddOption("再次開始",    [this]() { RestartLevel(); });         // index 2
    m_PauseMenu.AddOption("返回房間",    [this]() { ReturnToRoom(); });         // index 3

    m_CurrentState = State::UPDATE;
    TransitionTo(std::make_unique<TitleScreenState>());
}

void App::Update() {
    m_Root.Update();

    if (m_CurrentGameState) m_CurrentGameState->OnUpdate(*this);

    // ESC 或視窗關閉 → 結束程式 (暫停改回 ENTER；玩家 2 放炸彈改用右 Shift)
    if (Util::Input::IsKeyUp(Util::Keycode::ESCAPE) || Util::Input::IfExit()) {
        m_CurrentState = State::END;
    }

    if (m_CurrentState == State::UPDATE) ValidTask();
}

void App::End() { LOG_TRACE("End"); }

void App::ValidTask() { LOG_TRACE("ValidTask"); }

void App::StartMatch() {
    TransitionTo(std::make_unique<GameplayState>());
    m_Session.LoadLevel(m_SelectedLevel);
}

void App::ShowCover()       { m_Root.AddChild(m_CoverImage); }
void App::HideCover()       { m_Root.RemoveChild(m_CoverImage); }
void App::ShowMenuBg()      { m_Root.AddChild(m_MenuBg); }
void App::HideMenuBg()      { m_Root.RemoveChild(m_MenuBg); }
void App::ShowAttackerWin() { m_Root.AddChild(m_AttackImage); }
void App::ShowDefenderWin() { m_Root.AddChild(m_DefenseImage); }
void App::HideWinScreens() {
    m_Root.RemoveChild(m_AttackImage);
    m_Root.RemoveChild(m_DefenseImage);
}

// ----------------------------- Pause -----------------------------

void App::PauseGame()  { TransitionTo(std::make_unique<GamePausedState>()); }
void App::ResumeGame() { TransitionTo(std::make_unique<GameplayState>()); }

void App::RestartLevel() {
    LOG_INFO("Restarting level");
    m_Session.LoadLevel(m_Session.GetCurrentLevel());
    TransitionTo(std::make_unique<GameplayState>());
}

void App::ReturnToRoom() {
    LOG_INFO("Returning to battle setup");
    m_Session.Clear();
    TransitionTo(std::make_unique<BattleSetupState>());
}

void App::ShowPauseMenu() {
    m_PauseMenu.SetOptionLabel(kCheatOptionIndex,
        m_Session.IsCheatEnabled() ? "作弊模式：開" : "作弊模式：關");
    m_PauseMenu.Show(m_Root);
}
void App::HidePauseMenu()   { m_PauseMenu.Hide(m_Root); }
void App::UpdatePauseMenu() { m_PauseMenu.Update(); }
