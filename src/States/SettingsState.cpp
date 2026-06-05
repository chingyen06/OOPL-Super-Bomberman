#include "States/SettingsState.hpp"

#include "Core/App.hpp"
#include "States/MainMenuState.hpp"
#include "States/MenuCommon.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"

void SettingsState::OnEnter(App& app) {
    app.ShowMenuBg();
    m_Row = 0; m_Col = 0; m_Awaiting = false;
    auto& root = app.Root();

    m_Gear = std::make_shared<UIImage>(RESOURCE_DIR"/Image/gear.png", -560.0f, 333.0f, 40.0f);
    m_Gear->SetScale(0.5f, 0.5f);
    root.AddChild(m_Gear);
    m_Title = std::make_shared<UIText>("操作設定", -465.0f, 330.0f, 40.0f, MenuCommon::DarkText());
    root.AddChild(m_Title);

    // 背景音樂音量：分段式滑桿放在操作表底下 (可方向鍵 / 滑鼠拖曳)，並持久化。
    const float bgmY = 178.0f - kActions * 46.0f - 46.0f;  // 與 Build 的列排版一致
    m_BgmSlider.Show(root, 110.0f, bgmY, 300.0f, 20.0f);
    m_BgmSlider.SetValue(app.BgmVolume());
    m_BgmSlider.SetOnChange([&app](int v) { app.SetBgmVolume(v); });

    Build(app);
    m_Hint = MenuCommon::AddKeyHint(app, {{"方向鍵", "選擇/調整"}, {"空白鍵", "設定"}, {"Del", "清除"}, {"X", "返回"}});
}

void SettingsState::OnExit(App& app) {
    app.Keys().Save();  // 持久化本次改過的按鍵設定 (離開設定畫面時寫檔)
    auto& root = app.Root();
    ClearTable(root);
    m_BgmSlider.Hide(root);
    root.RemoveChild(m_Gear);
    root.RemoveChild(m_Title);
    m_Hint.Remove(app);
    app.HideMenuBg();
}

void SettingsState::Rebuild(App& app) {
    ClearTable(app.Root());
    Build(app);
}

void SettingsState::ClearTable(Util::Renderer& root) {
    m_Table.Clear(root);
}

void SettingsState::Build(App& app) {
    auto& root = app.Root();
    constexpr float kTableW = 1180.0f, kColLabel = -360.0f, kColP1 = 110.0f, kColP2 = 400.0f;
    constexpr float kHeaderY = 232.0f, kRow0Y = 178.0f, kStep = 46.0f;

    AddStrip(root, RESOURCE_DIR"/Image/set_row_b.png", kHeaderY, kTableW, 18.0f);
    AddText(root, "操作",          kColLabel, kHeaderY, MenuCommon::DarkText());
    AddText(root, "玩家 1 (防守)", kColP1,    kHeaderY, MenuCommon::DarkText());
    AddText(root, "玩家 2 (進攻)", kColP2,    kHeaderY, MenuCommon::DarkText());

    const char* names[kActions] = { "向上移動", "向下移動", "向左移動", "向右移動", "放置炸彈", "武器發動" };
    for (int i = 0; i < kActions; ++i) {
        const float y = kRow0Y - i * kStep;
        AddStrip(root, (i % 2 == 0) ? RESOURCE_DIR"/Image/set_row_a.png"
                                    : RESOURCE_DIR"/Image/set_row_b.png", y, kTableW, 18.0f);
        AddText(root, names[i], kColLabel, y, MenuCommon::DarkText());
        for (int col = 0; col < 2; ++col) {
            // 玩家2 沒有武器 → (武器發動, P2) 不可編輯，顯示「—」
            const bool editable = !(i == kActions - 1 && col == 1);
            const bool sel = (i == m_Row && col == m_Col) && editable;
            std::string label;
            if (!editable)            label = "—";
            else if (sel && m_Awaiting) label = "按鍵…";
            else                       label = MenuCommon::KeyName(app.Keys().Key(col, i));
            AddKeyBox(root, label, (col == 0 ? kColP1 : kColP2), y, sel);
        }
    }
    // 暫停列：兩名玩家共用的「單一」可設定鍵 (合併成一個，置於中央)
    const float pauseY = kRow0Y - kActions * kStep;
    const bool pauseSel = (m_Row == kPauseRow);
    AddStrip(root, RESOURCE_DIR"/Image/set_row_b.png", pauseY, kTableW, 18.0f);
    AddText(root, "暫停", kColLabel, pauseY, MenuCommon::DarkText());  // 標題不反白，選取提示靠右側鍵框
    const std::string pauseLabel = (pauseSel && m_Awaiting) ? "按鍵…" : MenuCommon::KeyName(app.Keys().pause);
    AddKeyBox(root, pauseLabel, (kColP1 + kColP2) * 0.5f, pauseY, pauseSel);

    // 背景音樂音量列：選取時整列底圖變橘 (用矩形橘條 row_sel，邊緣不會像 keycap 那樣彎);
    // 未選取則用一般列底圖。滑桿本身在 OnEnter 已掛上、跨 Rebuild 保留。
    const float bgmY = pauseY - kStep;
    const bool bgmSel = (m_Row == kBgmRow);
    AddStrip(root, bgmSel ? RESOURCE_DIR"/Image/row_sel.png" : RESOURCE_DIR"/Image/set_row_a.png",
             bgmY, kTableW, 18.0f);
    AddText(root, "背景音樂", kColLabel, bgmY, bgmSel ? MenuCommon::WhiteText() : MenuCommon::DarkText());
    m_BgmSlider.SetFocused(bgmSel);
}

void SettingsState::AddStrip(Util::Renderer& root, const std::string& img, float y, float w, float z) {
    auto s = std::make_shared<UIImage>(img, 0.0f, y, z);
    s->SetScale(w / 64.0f, 42.0f / 46.0f);
    m_Table.Add(root, s);
}

void SettingsState::AddText(Util::Renderer& root, const std::string& t, float x, float y, Util::Color c) {
    auto n = std::make_shared<UIText>(t, x, y, 20.0f, c);
    n->SetScale(0.62f, 0.62f);
    m_Table.Add(root, n);
}

// 按鍵方塊：selected→橘底深字，否則深底白字；key 空字串→空白方塊 (代表未綁定)
void SettingsState::AddKeyBox(Util::Renderer& root, const std::string& key, float cx, float y, bool selected) {
    constexpr float sc = 0.62f, trail = 14.0f * sc, pad = 12.0f;
    float w = 36.0f;  // 空白方塊預設寬
    std::shared_ptr<UIText> txt;
    if (!key.empty()) {
        txt = std::make_shared<UIText>(key, cx, y, 22.0f, selected ? MenuCommon::DarkText() : MenuCommon::WhiteText());
        txt->SetScale(sc, sc);
        w = txt->GetWidth() * sc - trail + pad * 2.0f;
    }
    const char* img = selected ? RESOURCE_DIR"/Image/keycap_sel.png" : RESOURCE_DIR"/Image/keycap_dark.png";
    auto box = std::make_shared<UIImage>(img, cx, y, 21.0f);
    box->SetScale(w / 60.0f, 32.0f / 42.0f);
    m_Table.Add(root, box);
    if (txt) {
        txt->SetPosition(cx + trail * 0.5f, y);
        m_Table.Add(root, txt);
    }
}

void SettingsState::OnUpdate(App& app) {
    using K = Util::Keycode;

    if (m_Awaiting) {
        // 等待新鍵：ESC 取消；偵測到任一鍵 → 綁定
        if (Util::Input::IsKeyDown(K::ESCAPE)) { m_Awaiting = false; Rebuild(app); return; }
        const Util::Keycode k = MenuCommon::PollAnyKey();
        if (k != KeyBindings::NoKey()) {
            if (m_Row == kPauseRow) {
                app.Keys().pause = k;  // 暫停為共用單一鍵，直接綁定
            } else {
                // 不允許重複綁定：把其他綁到同一鍵的格子清成未綁定
                for (int c = 0; c < 2; ++c) {
                    for (int a = 0; a < kActions; ++a) {
                        if (!(c == m_Col && a == m_Row) && app.Keys().Key(c, a) == k) {
                            app.Keys().Key(c, a) = KeyBindings::NoKey();
                        }
                    }
                }
                app.Keys().Key(m_Col, m_Row) = k;
            }
            // 綁的是空白/Enter → 吞掉它接下來的放開，否則會立刻又進入「重設」
            if (k == K::SPACE || k == K::RETURN) m_IgnoreConfirm = true;
            m_Awaiting = false;
            Rebuild(app);
        }
        return;
    }

    // (武器發動, 玩家2) 不可編輯 → 導覽略過
    auto editable = [](int row, int col) { return !(row == kActions - 1 && col == 1); };

    // 導覽支援方向鍵與 WASD；上下會在「操作列 + 聲音設定入口」之間循環。
    bool dirty = false;
    if (Util::Input::IsKeyUp(K::UP) || Util::Input::IsKeyUp(K::W)) {
        m_Row = (m_Row + kRows - 1) % kRows;
        if (m_Row < kActions && !editable(m_Row, m_Col)) m_Col = 0;  // 落在不可編輯格 → 跳回 P1
        dirty = true;
    }
    else if (Util::Input::IsKeyUp(K::DOWN) || Util::Input::IsKeyUp(K::S)) {
        m_Row = (m_Row + 1) % kRows;
        if (m_Row < kActions && !editable(m_Row, m_Col)) m_Col = 0;
        dirty = true;
    }
    else if (m_Row == kBgmRow) {
        // ---- 背景音樂列：方向鍵左右 ±5 調整音量 (滑桿自行更新、不需 Rebuild) ----
        if (Util::Input::IsKeyUp(K::LEFT) || Util::Input::IsKeyUp(K::A)) m_BgmSlider.Adjust(-5);
        else if (Util::Input::IsKeyUp(K::RIGHT) || Util::Input::IsKeyUp(K::D)) m_BgmSlider.Adjust(+5);
    }
    else if (m_Row == kPauseRow) {
        // ---- 暫停列：空白/Enter 設定鍵；Del 清除 ----
        if (Util::Input::IsKeyUp(K::SPACE) || Util::Input::IsKeyUp(K::RETURN)) {
            if (m_IgnoreConfirm) m_IgnoreConfirm = false;
            else { m_Awaiting = true; dirty = true; }
        }
        if (Util::Input::IsKeyUp(K::BACKSPACE) || Util::Input::IsKeyUp(K::DELETE)) {
            app.Keys().pause = KeyBindings::NoKey(); dirty = true;
        }
    }
    else {
        // ---- 操作列：左右換玩家欄 / 空白重設該鍵 / Del 清除 ----
        if (Util::Input::IsKeyUp(K::LEFT) || Util::Input::IsKeyUp(K::RIGHT) ||
            Util::Input::IsKeyUp(K::A)    || Util::Input::IsKeyUp(K::D)) {
            if (editable(m_Row, m_Col ^ 1)) { m_Col ^= 1; dirty = true; }  // 目標格不可編輯則不切換
        }
        const bool confirm = Util::Input::IsKeyUp(K::SPACE) || Util::Input::IsKeyUp(K::RETURN);
        if (confirm) {
            if (m_IgnoreConfirm) m_IgnoreConfirm = false;  // 吞掉剛綁定鍵的放開
            else if (editable(m_Row, m_Col)) { m_Awaiting = true; dirty = true; }
        }
        if ((Util::Input::IsKeyUp(K::BACKSPACE) || Util::Input::IsKeyUp(K::DELETE)) && editable(m_Row, m_Col)) {
            app.Keys().Key(m_Col, m_Row) = KeyBindings::NoKey();  // 清除→留空
            dirty = true;
        }
    }

    if (Util::Input::IsKeyUp(K::X)) { app.TransitionTo(std::make_unique<MainMenuState>()); return; }

    if (dirty) Rebuild(app);
}
