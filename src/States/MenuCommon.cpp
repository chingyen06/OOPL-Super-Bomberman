#include "States/MenuCommon.hpp"

#include "Core/App.hpp"
#include "Config/KeyBindings.hpp"
#include "Util/Input.hpp"

Util::Color DarkText()   { return Util::Color::FromName(Util::Colors::BLACK); }
Util::Color WhiteText()  { return Util::Color::FromName(Util::Colors::WHITE); }
Util::Color YellowText() { return Util::Color::FromName(Util::Colors::YELLOW); }
Util::Color GoldText()   { return Util::Color(230, 150, 20); }

// 按鍵 → 顯示字串。用 SDL scancode 範圍處理字母/數字，特殊鍵用 switch。
std::string KeyName(Util::Keycode k) {
    using K = Util::Keycode;
    if (k == KeyBindings::NoKey()) return "";
    const int sc = static_cast<int>(k);
    if (sc >= 4 && sc <= 29) return std::string(1, static_cast<char>('A' + (sc - 4)));   // A-Z
    if (sc >= 30 && sc <= 38) return std::string(1, static_cast<char>('1' + (sc - 30))); // 1-9
    if (sc == 39) return "0";
    switch (k) {
        case K::UP: return "↑"; case K::DOWN: return "↓"; case K::LEFT: return "←"; case K::RIGHT: return "→";
        case K::SPACE: return "空格鍵"; case K::RETURN: return "Enter"; case K::TAB: return "Tab";
        case K::RSHIFT: return "右Shift"; case K::LSHIFT: return "左Shift";
        case K::RCTRL: return "右Ctrl"; case K::LCTRL: return "左Ctrl"; case K::ESCAPE: return "ESC";
        default: return "?";
    }
}

Util::Keycode PollAnyKey() {
    using K = Util::Keycode;
    for (int sc = 4; sc <= 39; ++sc) {  // 字母 + 數字
        const auto k = static_cast<K>(sc);
        if (Util::Input::IsKeyDown(k)) return k;
    }
    const K specials[] = { K::UP, K::DOWN, K::LEFT, K::RIGHT, K::SPACE, K::RETURN,
                           K::RSHIFT, K::LSHIFT, K::RCTRL, K::LCTRL, K::TAB };
    for (const K k : specials) if (Util::Input::IsKeyDown(k)) return k;
    return KeyBindings::NoKey();
}

void KeyHint::Remove(App& app) {
    for (auto& c : caps)   app.Root().RemoveChild(c);
    for (auto& t : labels) app.Root().RemoveChild(t);
}

KeyHint AddKeyHint(App& app, const std::vector<std::pair<std::string, std::string>>& segs) {
    auto& root = app.Root();
    KeyHint hint;

    constexpr float scale      = 0.62f;
    constexpr float y          = -338.0f;
    constexpr float capPadX    = 12.0f;   // keycap 內側左右留白
    constexpr float keyActGap  = 6.0f;    // keycap 與動作字間距
    constexpr float segGap     = 22.0f;   // 段與段間距
    constexpr float capH       = 32.0f;   // keycap 螢幕高
    constexpr float rightEdge  = 622.0f;
    constexpr float capNativeW = 60.0f, capNativeH = 42.0f;
    constexpr float trailPad   = 14.0f * scale;  // 文字貼圖右側留白 (半值即置中補正)

    // keycap + 動作字一段的版面資料 (封裝；建構時算好字寬與膠囊寬)
    class Item {
    public:
        Item(const std::string& keyStr, const std::string& actStr,
             float scale, float trailPad, float capPadX, float y) {
            m_Key = std::make_shared<UIText>(keyStr, 0.0f, y, 60.0f, DarkText());
            m_Key->SetScale(scale, scale);
            m_Act = std::make_shared<UIText>(actStr, 0.0f, y, 60.0f,
                                             Util::Color::FromName(Util::Colors::DIM_GRAY));
            m_Act->SetScale(scale, scale);
            const float keyW = m_Key->GetWidth() * scale - trailPad;  // 扣掉右側留白才是實際字寬
            m_ActW = m_Act->GetWidth() * scale - trailPad;
            m_CapW = keyW + capPadX * 2.0f;
        }
        const std::shared_ptr<UIText>& Key() const { return m_Key; }
        const std::shared_ptr<UIText>& Act() const { return m_Act; }
        float ActW() const { return m_ActW; }
        float CapW() const { return m_CapW; }
    private:
        std::shared_ptr<UIText> m_Key, m_Act;
        float m_ActW, m_CapW;
    };
    std::vector<Item> items;
    float total = 0.0f;
    for (const auto& s : segs) {
        items.emplace_back(s.first, s.second, scale, trailPad, capPadX, y);
        total += items.back().CapW() + keyActGap + items.back().ActW() + segGap;
    }
    if (!items.empty()) total -= segGap;  // 末段不計尾距

    float cursor = rightEdge - total;  // 整列左緣
    for (const auto& it : items) {
        const float capCx = cursor + it.CapW() * 0.5f;
        auto cap = std::make_shared<UIImage>(RESOURCE_DIR"/Image/keycap.png", capCx, y, 59.0f);
        cap->SetScale(it.CapW() / capNativeW, capH / capNativeH);
        root.AddChild(cap);
        hint.caps.push_back(cap);

        it.Key()->SetPosition(capCx + trailPad * 0.5f, y);  // 往右補抵銷貼圖留白
        root.AddChild(it.Key());
        hint.labels.push_back(it.Key());
        cursor += it.CapW() + keyActGap;

        it.Act()->SetPosition(cursor + it.ActW() * 0.5f + trailPad * 0.5f, y);
        root.AddChild(it.Act());
        hint.labels.push_back(it.Act());
        cursor += it.ActW() + segGap;
    }
    return hint;
}

void CoinHud::Remove(App& app) {
    app.Root().RemoveChild(pill);
    app.Root().RemoveChild(icon);
    app.Root().RemoveChild(text);
}

CoinHud AddCoinHud(App& app) {
    auto pill = std::make_shared<UIImage>(RESOURCE_DIR"/Image/coin_pill.png", 540.0f, 322.0f, 49.0f);
    auto icon = std::make_shared<UIImage>(RESOURCE_DIR"/Image/coin.png", 498.0f, 322.0f, 50.0f);
    auto text = std::make_shared<UIText>(std::to_string(app.Profile().Coins()),
                                         556.0f, 322.0f, 50.0f, DarkText());
    text->SetScale(0.8f, 0.8f);
    app.Root().AddChild(pill);
    app.Root().AddChild(icon);
    app.Root().AddChild(text);
    return { pill, icon, text };
}
