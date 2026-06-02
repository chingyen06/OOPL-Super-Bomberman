#include <random>
#include "InteractableManager.hpp"
#include "Effects/PlayerEffects.hpp"
#include "Player.hpp"
#include "Util/Logger.hpp"

InteractableManager::InteractableManager() {
    // 55% 掉落空氣，15% 加速鞋、15% 炸彈道具、15% 火焰道具
    // 新增一種 powerup 只需在這裡加一行 — 不必動 PowerUp / Player / Factory 任何類別。
    m_LootTable.clear();
    m_LootTable.push_back({ 55, std::make_shared<EmptyDropFactory>() });
    m_LootTable.push_back({ 15, std::make_shared<GenericPowerUpFactory>(
        [] { return std::make_unique<SpeedBoostEffect>(); },
        RESOURCE_DIR"/Image/item_speedup.png") });
    m_LootTable.push_back({ 15, std::make_shared<GenericPowerUpFactory>(
        [] { return std::make_unique<BombUpEffect>(); },
        RESOURCE_DIR"/Image/item_bombup.png") });
    m_LootTable.push_back({ 15, std::make_shared<GenericPowerUpFactory>(
        [] { return std::make_unique<FirepowerUpEffect>(); },
        RESOURCE_DIR"/Image/item_fireup.png") });

    // 地圖字元 → interactable 的對應表
    // 新增一種 interactable 只需新增一個 RegisterSymbol — 不必再動 LevelManager。
    RegisterSymbol('K', [this](int x, int y, Util::Renderer& r) { AddKey(x, y, r); });
    RegisterSymbol('9', [this](int x, int y, Util::Renderer& r) { AddChest(x, y, r); });
    RegisterSymbol('U', [this](int x, int y, Util::Renderer& r) { AddConveyor(x, y, Direction::UP,    r); });
    RegisterSymbol('D', [this](int x, int y, Util::Renderer& r) { AddConveyor(x, y, Direction::DOWN,  r); });
    RegisterSymbol('L', [this](int x, int y, Util::Renderer& r) { AddConveyor(x, y, Direction::LEFT,  r); });
    RegisterSymbol('R', [this](int x, int y, Util::Renderer& r) { AddConveyor(x, y, Direction::RIGHT, r); });
    RegisterSymbol('4', [this](int x, int y, Util::Renderer& r) { AddBouncePad(x, y, Direction::UP,    r); });
    RegisterSymbol('5', [this](int x, int y, Util::Renderer& r) { AddBouncePad(x, y, Direction::DOWN,  r); });
    RegisterSymbol('6', [this](int x, int y, Util::Renderer& r) { AddBouncePad(x, y, Direction::LEFT,  r); });
    RegisterSymbol('7', [this](int x, int y, Util::Renderer& r) { AddBouncePad(x, y, Direction::RIGHT, r); });
}

void InteractableManager::RegisterSymbol(char symbol, SymbolHandler handler) {
    m_SymbolTable[symbol] = std::move(handler);
}

bool InteractableManager::HandleSymbol(char symbol, int gridX, int gridY, Util::Renderer& root) {
    auto it = m_SymbolTable.find(symbol);
    if (it == m_SymbolTable.end()) return false;
    it->second(gridX, gridY, root);
    return true;
}

void InteractableManager::AddKey(int gridX, int gridY, Util::Renderer& root) {
    auto key = std::make_shared<Key>(gridX, gridY);
    m_Interactables.push_back(key);
    root.AddChild(key);
}

void InteractableManager::AddChest(int gridX, int gridY, Util::Renderer& root) {
    auto chest = std::make_shared<Chest>(gridX, gridY);
    m_Interactables.push_back(chest);
    root.AddChild(chest);
}

void InteractableManager::Update(std::vector<std::shared_ptr<Player>>& players, Util::Renderer& root) {
    for (auto it = m_Interactables.begin(); it != m_Interactables.end(); ) {
        auto item = *it;
        item->Update();
        bool itemErased = false;

        for (auto& player : players) {
            if (player->IsDead())
                continue;

            if (player->GetGridX() == item->GetGridX() && player->GetGridY() == item->GetGridY()) {

                if (item->OnInteract(*player)) {
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
}

void InteractableManager::Clear(Util::Renderer& root) {
    for (auto& interactable : m_Interactables) {
        root.RemoveChild(interactable);
    }
    m_Interactables.clear();
}

bool InteractableManager::IsBlocksBombAt(int gridX, int gridY) const {
    for (const auto& item : m_Interactables) {
        if (item->GetGridX() == gridX && item->GetGridY() == gridY) {
            return item->IsBlocksBomb();
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

bool InteractableManager::AreAllObjectivesComplete() const {
    for (const auto& item : m_Interactables) {
        if (item->IsScoringObjective() && !item->IsObjectiveComplete()) {
            return false;
        }
    }
    return true;
}

void InteractableManager::ForceCompleteObjectives() {
    for (auto& item : m_Interactables) {
        if (item->IsScoringObjective() && !item->IsObjectiveComplete()) {
            item->ForceComplete();
        }
    }
}

int InteractableManager::GetObjectiveCount() const {
    int count = 0;
    for (const auto& item : m_Interactables) {
        if (item->IsScoringObjective()) {
            count++;
        }
    }
    return count;
}

std::vector<bool> InteractableManager::GetObjectiveStatusList() const {
    std::vector<bool> statusList;
    for (const auto& item : m_Interactables) {
        if (item->IsScoringObjective()) {
            statusList.push_back(item->IsObjectiveComplete());
        }
    }
    return statusList;
}

void InteractableManager::DestroyFlammableAt(int gridX, int gridY, Util::Renderer& root) {
    for (auto it = m_Interactables.begin(); it != m_Interactables.end(); ) {
        if ((*it)->GetGridX() == gridX && (*it)->GetGridY() == gridY && (*it)->IsDestroyedByFire()) {
            root.RemoveChild(*it);
            it = m_Interactables.erase(it);
        }
        else {
            ++it;
        }
    }
}

void InteractableManager::OnBrickDestroyed(int gridX, int gridY, Util::Renderer& root) {
    int totalWeight = 0;
    for (const auto& entry : m_LootTable) {
        totalWeight += entry.weight;
    }

    // 亂數
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

glm::vec2 InteractableManager::GetForceAt(int gridX, int gridY) const {
    glm::vec2 totalForce = { 0.0f, 0.0f };
    for (const auto& item : m_Interactables) {
        if (item->GetGridX() == gridX && item->GetGridY() == gridY) {
            totalForce.x += item->GetForce().x;
            totalForce.y += item->GetForce().y;
        }
    }
    return totalForce;
}

void InteractableManager::AddConveyor(int gridX, int gridY, Direction dir, Util::Renderer& root) {
    auto conveyor = std::make_shared<Conveyor>(gridX, gridY, dir);
    m_Interactables.push_back(conveyor);
    root.AddChild(conveyor);
}

void InteractableManager::AddBouncePad(int gridX, int gridY, Direction dir, Util::Renderer& root) {
    auto bouncePad = std::make_shared<BouncePad>(gridX, gridY, dir);
    m_Interactables.push_back(bouncePad);
    root.AddChild(bouncePad);
}