#ifndef LEVELSELECTSTATE_HPP
#define LEVELSELECTSTATE_HPP

#include <memory>

#include <vector>

#include "States/IGameState.hpp"
#include "States/MenuCommon.hpp"
#include "UI/UIButtonList.hpp"
#include "UI/UIImage.hpp"
#include "UI/UIText.hpp"
#include "Util/Image.hpp"

// 選擇關卡：上方顯示選中關卡的縮圖預覽，下方為名稱按鈕列；左右選擇、空白鍵確定。
class LevelSelectState : public IGameState {
public:
    void OnEnter(App& app) override;
    void OnExit(App& app) override;
    void OnUpdate(App& app) override;

private:
    UIButtonList m_Thumbs;
    std::shared_ptr<UIText>  m_Title;
    std::shared_ptr<UIImage> m_Preview;                       // 大張預覽
    std::vector<std::shared_ptr<Util::Image>> m_PreviewImgs;  // 各關縮圖 (預載)
    int m_LastSel = -1;
    KeyHint m_Hint;
};

#endif
