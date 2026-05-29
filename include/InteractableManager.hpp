#ifndef INTERACTABLEMANAGER_HPP
#define INTERACTABLEMANAGER_HPP

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>
#include "Interactable.hpp"
#include "Player.hpp"
#include "Util/Renderer.hpp"
#include "Util/Logger.hpp"

class Player;
class InteractableFactory;

struct LootEntry {
    int weight; // 掉落權重
    std::shared_ptr<InteractableFactory> factory;
};

class InteractableManager {
public:
    // 註冊表用 — char (地圖符號) → 建立 interactable
    using SymbolHandler = std::function<void(int gridX, int gridY, Util::Renderer&)>;

    InteractableManager();

    // 所有 Add* 方法皆會 push 進清單並立刻 AddChild 到 root，
    // 確保載入期/遊戲中行為一致 (不再需要呼叫端記得 AttachToRoot)
    void AddKey(int gridX, int gridY, Util::Renderer& root);
    void AddChest(int gridX, int gridY, Util::Renderer& root);
    void AddConveyor(int gridX, int gridY, Direction dir, Util::Renderer& root);
    void AddBouncePad(int gridX, int gridY, Direction dir, Util::Renderer& root);

    // LevelManager 從地圖檔讀到字元時呼叫 HandleSymbol，
    // 由這個 manager 查表決定如何建立對應 interactable。
    // 新增一種 interactable 只需在 InteractableManager 建構子裡 RegisterSymbol — 不必動 LevelManager。
    void RegisterSymbol(char symbol, SymbolHandler handler);
    bool HandleSymbol(char symbol, int gridX, int gridY, Util::Renderer& root);

    void Update(std::vector<std::shared_ptr<Player>>& players, Util::Renderer& root);

    void Clear(Util::Renderer& root);

    bool IsBlocksBombAt(int gridX, int gridY) const;
    bool BlocksFireAt(int gridX, int gridY) const;

    // 勝負/HUD 查詢：以 Interactable 的 IsScoringObjective/IsObjectiveComplete hook 為準，
    // 不再對 Chest 做 dynamic_cast — 新增目標型別不必改這裡。
    bool AreAllObjectivesComplete() const;

    int GetObjectiveCount() const;

    std::vector<bool> GetObjectiveStatusList() const;

    // 只允許外部讀取，不再外露 non-const 容器
    const std::vector<std::shared_ptr<Interactable>>& GetInteractables() const { return m_Interactables; }

    // 領域 API：銷毀指定格上「被火焰摧毀」的互動物件 (BombManager 爆炸時呼叫)
    void DestroyFlammableAt(int gridX, int gridY, Util::Renderer& root);

    void OnBrickDestroyed(int gridX, int gridY, Util::Renderer& root);

    glm::vec2 GetForceAt(int gridX, int gridY) const;

private:
    std::vector<std::shared_ptr<Interactable>> m_Interactables;

    std::vector<LootEntry> m_LootTable;  // 掉落表
    std::unordered_map<char, SymbolHandler> m_SymbolTable;  // 地圖字元 → 建構策略
};

#endif