#ifndef CONFIRMDIALOG_HPP
#define CONFIRMDIALOG_HPP

#include <memory>
#include <string>

#include "UI/UIImage.hpp"
#include "UI/UIText.hpp"
#include "Util/Image.hpp"
#include "Util/Renderer.hpp"

// 通用「是/否」確認對話框 (仿原版結束遊戲確認)。左右選擇、空格確定、X/ESC 視為「否」。
// 自行管理圖層 (變暗 + 白底面板 + 漸層標題 + 兩顆按鈕)。
class ConfirmDialog {
public:
    enum class Result { Pending, Yes, No };

    void Init();  // 預載按鈕底圖 (GL context 就緒後)
    void Show(Util::Renderer& root, const std::string& question);
    void Hide(Util::Renderer& root);
    Result Update();  // 處理左右/確定；回傳是否已決定
    bool IsVisible() const { return m_Visible; }

private:
    void UpdateCursor();

    static constexpr float kZDim = 97.0f, kZPanel = 98.0f, kZBtn = 99.0f, kZText = 100.0f;

    bool m_Visible = false;
    int  m_Sel = 1;  // 0 = 是, 1 = 否 (預設停在「否」較安全)

    std::shared_ptr<Util::Image> m_BtnNormal, m_BtnSelected;

    std::shared_ptr<UIImage> m_Dim, m_Panel, m_TitleBar, m_YesBtn, m_NoBtn;
    std::shared_ptr<UIText>  m_Title, m_Question, m_YesLabel, m_NoLabel;
};

#endif
