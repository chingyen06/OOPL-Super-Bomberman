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

    void AddKey(int gridX, int gridY);
    void AddChest(int gridX, int gridY);

    void Update(std::vector<std::shared_ptr<Player>>& players, Util::Renderer& root);

    void Clear(Util::Renderer& root);

    bool IsInteractableAt(int gridX, int gridY) const;
    bool BlocksFireAt(int gridX, int gridY) const;

    void AttachToRoot(Util::Renderer& root);

    bool IsAllChestOpened() const;

    void DropKey(int gridX, int gridY, Util::Renderer& root);

    int GetTotalChestCount() const;

    std::vector<bool> GetChestStatusList() const;

    std::vector<std::shared_ptr<Interactable>>& GetInteractables() { return m_Interactables; }
    const std::vector<std::shared_ptr<Interactable>>& GetInteractables() const { return m_Interactables; }

    void RemoveItem(std::vector<std::shared_ptr<Interactable>>::iterator& it, Util::Renderer& root);

    void OnBrickDestroyed(int gridX, int gridY, Util::Renderer& root);

private:
    /*std::vector<std::shared_ptr<Key>> m_Keys;
    std::vector<std::shared_ptr<Chest>> m_Chests;*/

    std::vector<std::shared_ptr<Interactable>> m_Interactables;
    std::vector<bool> m_ChestStatusCache;  // 內部快取
    void UpdateChestStatusCache();

	std::vector<LootEntry> m_LootTable;  // 掉落表
};

#endif