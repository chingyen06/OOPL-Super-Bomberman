#ifndef INTERACTABLEMANAGER_HPP
#define INTERACTABLEMANAGER_HPP

#include <vector>
#include <memory>
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
    InteractableManager();

    // 所有 Add* 方法皆會 push 進清單並立刻 AddChild 到 root，
    // 確保載入期/遊戲中行為一致 (不再需要呼叫端記得 AttachToRoot)
    void AddKey(int gridX, int gridY, Util::Renderer& root);
    void AddChest(int gridX, int gridY, Util::Renderer& root);
    void AddConveyor(int gridX, int gridY, Direction dir, Util::Renderer& root);
    void AddBouncePad(int gridX, int gridY, Direction dir, Util::Renderer& root);

    void Update(std::vector<std::shared_ptr<Player>>& players, Util::Renderer& root);

    void Clear(Util::Renderer& root);

    bool IsBlocksBombAt(int gridX, int gridY) const;
    bool BlocksFireAt(int gridX, int gridY) const;

    bool IsAllChestOpened() const;

    int GetTotalChestCount() const;

    std::vector<bool> GetChestStatusList() const;

    // 只允許外部讀取，不再外露 non-const 容器
    const std::vector<std::shared_ptr<Interactable>>& GetInteractables() const { return m_Interactables; }

    // 領域 API：銷毀指定格上「被火焰摧毀」的互動物件 (BombManager 爆炸時呼叫)
    void DestroyFlammableAt(int gridX, int gridY, Util::Renderer& root);

    void OnBrickDestroyed(int gridX, int gridY, Util::Renderer& root);

    glm::vec2 GetForceAt(int gridX, int gridY) const;

private:
    /*std::vector<std::shared_ptr<Key>> m_Keys;
    std::vector<std::shared_ptr<Chest>> m_Chests;*/

    std::vector<std::shared_ptr<Interactable>> m_Interactables;
    std::vector<bool> m_ChestStatusCache;  // 內部快取
    void UpdateChestStatusCache();

	std::vector<LootEntry> m_LootTable;  // 掉落表
};

#endif