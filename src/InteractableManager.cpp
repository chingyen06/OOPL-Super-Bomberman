#include "InteractableManager.hpp"
#include "Player.hpp"
#include "Util/Logger.hpp"

void InteractableManager::AddKey(int gridX, int gridY) {
    auto key = std::make_shared<Key>(gridX, gridY);
    m_Keys.push_back(key);
}

void InteractableManager::AddChest(int gridX, int gridY) {
    auto chest = std::make_shared<Chest>(gridX, gridY);
    m_Chests.push_back(chest);
}

void InteractableManager::AttachToRoot(Util::Renderer& root) {
    for (auto& key : m_Keys) root.AddChild(key);
    for (auto& chest : m_Chests) root.AddChild(chest);
}

void InteractableManager::Update(std::shared_ptr<Player>& player, Util::Renderer& root) {
    if (player->IsDead()) 
        return;

    // 鑰匙拾取判定
    for (auto it = m_Keys.begin(); it != m_Keys.end(); ) {
        if (player->GetGridX() == (*it)->GetGridX() &&
            player->GetGridY() == (*it)->GetGridY() && !player->HasKey()) {

            player->SetKey(true); // 玩家獲得鑰匙
            LOG_INFO("Player picked up the Key!");

            root.RemoveChild(*it);
            it = m_Keys.erase(it);
        }
        else {
            ++it;
        }
    }

    // 寶箱開啟判定
    for (auto& chest : m_Chests) {
        if (!chest->IsOpened() &&
            player->GetGridX() == chest->GetGridX() &&
            player->GetGridY() == chest->GetGridY()) {

            if (player->HasKey()) {
                chest->Open();
                player->SetKey(false);
                LOG_INFO("Chest Opened! Check victory condition here.");
            }
        }
    }
}

void InteractableManager::Clear(Util::Renderer& root) {
    for (auto& k : m_Keys) 
        root.RemoveChild(k);
    for (auto& c : m_Chests) 
        root.RemoveChild(c);

    m_Keys.clear();
    m_Chests.clear();
}

bool InteractableManager::IsInteractableAt(int gridX, int gridY) const {
    for (const auto& chest : m_Chests) {
        if (chest->GetGridX() == gridX && chest->GetGridY() == gridY) {
            return true;
        }
    }
    for (const auto& key : m_Keys) {
        if (key->GetGridX() == gridX && key->GetGridY() == gridY) {
            return true;
        }
    }

    return false;
}

bool InteractableManager::IsAllChestOpened() const {
    for (const auto& chest : m_Chests) {
        if (!chest->IsOpened()) {
            return false;
        }
    }

    return true;
}