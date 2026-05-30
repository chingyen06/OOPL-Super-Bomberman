#ifndef MENUCOMMON_HPP
#define MENUCOMMON_HPP

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "UI/UIImage.hpp"
#include "UI/UIText.hpp"
#include "Util/Color.hpp"
#include "Util/Keycode.hpp"

class App;

// 各選單畫面共用的小工具 (字色、按鍵名稱、右下角提示、右上角金幣)。
// 抽出成獨立單元 (SRP)，避免每個 State 重複實作、也讓 App.cpp 不再臃腫。

// 共用字色 (淺色背景用深字)
Util::Color DarkText();
Util::Color WhiteText();
Util::Color YellowText();
Util::Color GoldText();   // 結算「+金幣」用

// 按鍵 → 顯示字串 (未綁定回空字串)；以及掃描本幀剛按下的鍵 (供重新綁定)
std::string   KeyName(Util::Keycode k);
Util::Keycode PollAnyKey();

// 文字貼圖右側留白 → 往右補正才水平置中 (量測值)
inline constexpr float kLabelXNudge = 7.0f;
inline constexpr float kLabelYNudge = 1.0f;

// 仿原版：右下角按鍵提示。每段 = (按鍵字, 動作字)，按鍵以淺色 keycap 框呈現，靠右對齊。
class KeyHint {
public:
    std::vector<std::shared_ptr<UIImage>> caps;
    std::vector<std::shared_ptr<UIText>>  labels;
    void Remove(App& app);
};
KeyHint AddKeyHint(App& app, const std::vector<std::pair<std::string, std::string>>& segs);

// 仿原版：右上角金幣顯示 (白色膠囊 + 金幣圖示 + 深色數字)。
class CoinHud {
public:
    std::shared_ptr<UIImage> pill;
    std::shared_ptr<UIImage> icon;
    std::shared_ptr<UIText>  text;
    void Remove(App& app);
};
CoinHud AddCoinHud(App& app);

#endif
