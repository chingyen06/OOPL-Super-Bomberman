#ifndef RESULTSSTATE_HPP
#define RESULTSSTATE_HPP

#include <memory>
#include <vector>

#include "States/IGameState.hpp"
#include "UI/UIText.hpp"

// 結算畫面：疊在勝利圖之上顯示「本回戰鬥 +金幣 / 總計」；空白鍵返回主選單。
class ResultsState : public IGameState {
public:
    void OnEnter(App& app) override;
    void OnExit(App& app) override;
    void OnUpdate(App& app) override;

private:
    std::vector<std::shared_ptr<UIText>> m_Nodes;
};

#endif
