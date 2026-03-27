#include "InteractableManager.hpp"
#include "Player.hpp"
#include "Util/Logger.hpp"

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

            if (player->GetGridX() == item->GetGridX() && player->GetGridY() == item->GetGridY() && player->GetTeam() == Team::ATTACKER) {

                if (auto key = std::dynamic_pointer_cast<Key>(item)) {
                    if (!player->HasKey()) {
                        player->SetKey(true);
                        LOG_INFO("Player picked up the Key!");

                        root.RemoveChild(item);
                        it = m_Interactables.erase(it);

                        itemErased = true;
                        break;
                    }
                }
                else if (auto chest = std::dynamic_pointer_cast<Chest>(item)) {
                    if (!chest->IsOpened() && player->HasKey()) {
                        chest->Open();
                        player->SetKey(false);
                        LOG_INFO("Chest Opened! Check victory condition here.");
                    }
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