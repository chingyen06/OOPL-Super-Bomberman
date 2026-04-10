#include <random>
#include "InteractableManager.hpp"
#include "Player.hpp"
#include "Util/Logger.hpp"

InteractableManager::InteractableManager() {
    // 70% 掉落空氣，30% 掉落加速鞋
    m_LootTable.push_back({ 70, std::make_shared<EmptyDropFactory>() });
    m_LootTable.push_back({ 30, std::make_shared<SpeedItemFactory>() });
}

void InteractableManager::AddKey(int gridX, int gridY) {
    auto key = std::make_shared<Key>(gridX, gridY);
    m_Interactables.push_back(key);
}

void InteractableManager::AddChest(int gridX, int gridY) {
    auto chest = std::make_shared<Chest>(gridX, gridY);
    m_Interactables.push_back(chest);
}

void InteractableManager::AttachToRoot(Util::Renderer& root) {
    for (auto& interactable : m_Interactables) {
        root.AddChild(interactable);
    }
}

void InteractableManager::Update(std::vector<std::shared_ptr<Player>>& players, Util::Renderer& root) {
    for (auto it = m_Interactables.begin(); it != m_Interactables.end(); ) {
        auto item = *it;
        bool itemErased = false;

        for (auto& player : players) {
            if (player->IsDead())
                continue;

            if (player->GetGridX() == item->GetGridX() && player->GetGridY() == item->GetGridY()) {

                if (item->OnInteract(player)) {
                    root.RemoveChild(item);
                    it = m_Interactables.erase(it);
                    itemErased = true;
                    break;
                }
            }
        }

        if (!itemErased) {
            ++it;
        }
    }

    UpdateChestStatusCache();
}

void InteractableManager::Clear(Util::Renderer& root) {
    for (auto& interactable : m_Interactables) {
        root.RemoveChild(interactable);
    }
    m_Interactables.clear();
}

bool InteractableManager::IsInteractableAt(int gridX, int gridY) const {
    for (const auto& interactable : m_Interactables) {
        if (interactable->GetGridX() == gridX && interactable->GetGridY() == gridY) {
            return true;
        }
    }

    return false;
}

bool InteractableManager::BlocksFireAt(int gridX, int gridY) const {
    for (const auto& interactable : m_Interactables) {
        if (interactable->GetGridX() == gridX && interactable->GetGridY() == gridY) {
            return interactable->IsBlocksFire();
        }
    }

	return false;
}

bool InteractableManager::IsAllChestOpened() const {
    for (const auto& item : m_Interactables) {
        if (auto chest = std::dynamic_pointer_cast<Chest>(item)) {
            if (!chest->IsOpened()) {
                return false;
            }
        }
    }
    return true;
}

void InteractableManager::DropKey(int gridX, int gridY, Util::Renderer& root) {
    auto key = std::make_shared<Key>(gridX, gridY);
    m_Interactables.push_back(key);

    root.AddChild(key);
}

int InteractableManager::GetTotalChestCount() const {
    int count = 0;
    for (const auto& item : m_Interactables) {
        if (std::dynamic_pointer_cast<Chest>(item)) {
            count++;
        }
    }
    return count;
}

std::vector<bool> InteractableManager::GetChestStatusList() const {
    std::vector<bool> statusList;
    for (const auto& item : m_Interactables) {
        if (auto chest = std::dynamic_pointer_cast<Chest>(item)) {
            statusList.push_back(chest->IsOpened());
        }
    }
    return statusList;
}

void InteractableManager::UpdateChestStatusCache() {
    m_ChestStatusCache.clear(); // 保留 Capacity 的高效清空
    for (const auto& item : m_Interactables) {
        if (auto chest = std::dynamic_pointer_cast<Chest>(item)) {
            m_ChestStatusCache.push_back(chest->IsOpened());
        }
    }
}

void InteractableManager::RemoveItem(std::vector<std::shared_ptr<Interactable>>::iterator& it, Util::Renderer& root) {
    root.RemoveChild(*it);
    it = m_Interactables.erase(it);
}

void InteractableManager::OnBrickDestroyed(int gridX, int gridY, Util::Renderer& root) {
    int totalWeight = 0;
    for (const auto& entry : m_LootTable) {
        totalWeight += entry.weight;
    }

    // 亂數引擎
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(1, totalWeight);
    int roll = dist(gen);

    // 隨機抽
    for (const auto& entry : m_LootTable) {
        roll -= entry.weight;
        if (roll <= 0) {
            auto item = entry.factory->Create(gridX, gridY);
            if (item != nullptr) {
                m_Interactables.push_back(item);
                root.AddChild(item);
            }
            break;
        }
    }
}