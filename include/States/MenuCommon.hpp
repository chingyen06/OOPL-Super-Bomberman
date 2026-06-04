#ifndef MENUCOMMON_HPP
#define MENUCOMMON_HPP

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "UI/UIGroup.hpp"
#include "UI/UIImage.hpp"
#include "UI/UIText.hpp"
#include "Util/Color.hpp"
#include "Util/Keycode.hpp"

class App;

// 文字貼圖右側留白 → 往右補正才水平置中 (量測值)。
inline constexpr float kLabelXNudge = 7.0f;
inline constexpr float kLabelYNudge = 1.0f;

// 仿原版：右下角按鍵提示。內部以 UIGroup 持有整組節點 (整批進出場景)。
class KeyHint {
public:
    void Add(Util::Renderer& root, const std::shared_ptr<Util::GameObject>& node) { m_Group.Add(root, node); }
    void Remove(App& app);  // 從場景移除整組提示
private:
    UIGroup m_Group;
};

// 仿原版：右上角金幣顯示 (白色膠囊 + 金幣圖示 + 深色數字)。
class CoinHud {
public:
    void Add(Util::Renderer& root, const std::shared_ptr<Util::GameObject>& node) { m_Group.Add(root, node); }
    void Remove(App& app);
private:
    UIGroup m_Group;
};

// 各選單畫面共用的小工具：字色、按鍵名稱、右下角提示、右上角金幣。
// 傳統 OOP：以 class + static 方法承載 (取代散落的自由函式)，用法與 GridCoord:: / Constants::
// 一致 (e.g. MenuCommon::DarkText())。純靜態工具類，不實例化。
class MenuCommon {
public:
    // 共用字色 (淺色背景用深字)
    static Util::Color DarkText();
    static Util::Color WhiteText();
    static Util::Color YellowText();
    static Util::Color GoldText();   // 結算「+金幣」用

    // 按鍵 → 顯示字串 (未綁定回空字串)；以及掃描本幀剛按下的鍵 (供重新綁定)
    static std::string   KeyName(Util::Keycode k);
    static Util::Keycode PollAnyKey();

    // 工廠：建立提示 / 金幣顯示並掛上場景，回傳供 state 持有 (之後以 .Remove() 拆除)。
    static KeyHint AddKeyHint(App& app, const std::vector<std::pair<std::string, std::string>>& segs);
    static CoinHud AddCoinHud(App& app);

private:
    MenuCommon() = default;  // 純靜態工具類，不需實例化

    // keycap + 動作字一段的版面資料 (建構時算好字寬與膠囊寬)。AddKeyHint 專用。
    // 抽自原本寫在 .cpp 內的函式區域類別，改放 header 的 private 巢狀類別 (class 不寫在 cpp)。
    class Item {
    public:
        Item(const std::string& keyStr, const std::string& actStr,
             float scale, float trailPad, float capPadX, float y);
        const std::shared_ptr<UIText>& Key() const { return m_Key; }
        const std::shared_ptr<UIText>& Act() const { return m_Act; }
        float ActW() const { return m_ActW; }
        float CapW() const { return m_CapW; }
    private:
        std::shared_ptr<UIText> m_Key, m_Act;
        float m_ActW, m_CapW;
    };
};

#endif
