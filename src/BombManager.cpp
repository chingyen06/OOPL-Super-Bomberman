#include "BombManager.hpp"
#include "GameConstants.hpp"
#include "GameTypes.hpp"
#include "GridCoord.hpp"
#include "InteractableManager.hpp"
#include "Player.hpp"
#include "Util/Logger.hpp"
#include <cmath>

// Place a bomb on the player's current grid
void BombManager::PlaceBomb(Player& player, LevelManager& levelManager, InteractableManager& interactableManager, Util::Renderer& root, const std::vector<std::shared_ptr<Player>>& players) {
    if (!player.CanPlaceBomb())  // Player has reached their max bomb count
        return;

    // Compute the player's grid coordinate
    int targetX = player.GetGridX();
    int targetY = player.GetGridY();

    float bombPixelX = GridCoord::ToPixelX(targetX);
    float bombPixelY = GridCoord::ToPixelY(targetY);

    for (const auto& p : players) {
        if (p->GetPlayerID() == player.GetPlayerID() || p->IsDead()) continue;
        if (std::abs(p->GetPixelPos().x - bombPixelX) < GridCoord::kTileSize && std::abs(p->GetPixelPos().y - bombPixelY) < GridCoord::kTileSize) {
            LOG_INFO("Cannot place bomb: Another player is occupying the grid.");
            return;
        }
    }

    // The target tile must be walkable, free of bombs, and not blocked by an interactable
    if (levelManager.IsWalkable(targetX, targetY) && !IsBombAt(targetX, targetY) && !interactableManager.IsBlocksBombAt(targetX, targetY)) {
        auto newBomb = std::make_shared<Bomb>(targetX, targetY, player.GetFirepower(), player.GetPlayerID());
        m_Bombs.push_back(newBomb);
        root.AddChild(newBomb);

        player.AddBombCount(); // Increment placed-bomb counter

        for (const auto& p : players) {  // Make nearby players temporarily ignore this bomb so they aren't stuck on top of it
            if (p->IsDead()) continue;
            if (std::abs(p->GetPixelPos().x - bombPixelX) < Constants::Bomb::kIgnoreClearance &&
                std::abs(p->GetPixelPos().y - bombPixelY) < Constants::Bomb::kIgnoreClearance) {
                p->SetIgnoreBomb(targetX, targetY);
            }
        }

        LOG_INFO("Bomb placed at (" + std::to_string(targetX) + ", " + std::to_string(targetY) + ")");
    }
}

// Update bombs and explosions
void BombManager::Update(LevelManager& levelManager, InteractableManager& interactableManager, Util::Renderer& root, std::vector<std::shared_ptr<Player>>& players) {
    for (auto it = m_Bombs.begin(); it != m_Bombs.end(); ) {  // Bomb lifecycle and spawning explosions
        (*it)->Update(levelManager, *this, interactableManager);

        if ((*it)->GetState() == Bomb::State::DONE) {
            int bx = (*it)->GetGridX();
            int by = (*it)->GetGridY();
            int fp = (*it)->GetFirepower();

            // Centre explosion tile
            auto centerFire = std::make_shared<Explosion>(bx, by);
            m_Explosions.push_back(centerFire);
            root.AddChild(centerFire);

            // Cross-shaped explosion in the 4 cardinal directions
            for (const auto& off : kCardinalOffsets) {
                for (int step = 1; step <= fp; step++) {
                    int targetX = bx + off.dx * step;
                    int targetY = by + off.dy * step;

                    if (!levelManager.IsWalkable(targetX, targetY)) {
                        if (levelManager.IsBrick(targetX, targetY)) {
                            // Spawn a flame on the brick itself
                            auto fire = std::make_shared<Explosion>(targetX, targetY);
                            m_Explosions.push_back(fire);
                            root.AddChild(fire);

                            // Destroy the brick (may drop loot)
                            levelManager.DestroyBrick(targetX, targetY, root, interactableManager);
                        }
                        break;
                    }

                    // Destroy any flammable interactables in this tile before placing the flame
                    interactableManager.DestroyFlammableAt(targetX, targetY, root);

                    auto fire = std::make_shared<Explosion>(targetX, targetY);
                    m_Explosions.push_back(fire);
                    root.AddChild(fire);
                }
            }


            for (auto& player : players) {
                if (player->GetPlayerID() == (*it)->GetOwnerID()) {
                    player->DecBombCount();
                    break;
                }
            }

            root.RemoveChild(*it);
            it = m_Bombs.erase(it);
        }
        else {
            ++it;
        }
    }

    // Damage resolution for current explosions
    for (auto it = m_Explosions.begin(); it != m_Explosions.end(); ) {
        (*it)->Update();

        // Player burn check — 用像素 AABB 而非格子整數比對，避免邊緣擦肩漏判
        const float flameCx = GridCoord::ToPixelX((*it)->GetGridX());
        const float flameCy = GridCoord::ToPixelY((*it)->GetGridY());
        constexpr float kFlameHalfSize = GridCoord::kTileSize * 0.5f;
        constexpr float kPlayerHalfSize = Constants::Player::kCollisionRadius;
        for (auto& player : players) {
            if (player->IsDead()) continue;
            const auto pp = player->GetPixelPos();
            if (std::abs(pp.x - flameCx) < kFlameHalfSize + kPlayerHalfSize &&
                std::abs(pp.y - flameCy) < kFlameHalfSize + kPlayerHalfSize) {
                player->Kill();
                LOG_INFO("Player was BURNED!");
            }
        }

        // Chain-reaction trigger: any counting-down bomb in this flame tile detonates immediately
        for (auto& bomb : m_Bombs) {
            if (bomb->GetState() == Bomb::State::COUNTDOWN &&
                bomb->GetGridX() == (*it)->GetGridX() &&
                bomb->GetGridY() == (*it)->GetGridY()) {

                bomb->ForceDetonate(); // Force the bomb to enter DONE state next frame
                LOG_INFO("Chain Reaction Triggered!");
            }
        }

        if ((*it)->IsDone()) {
            root.RemoveChild(*it);
            it = m_Explosions.erase(it);
        }
        else {
            ++it;
        }
    }
}

void BombManager::Clear(Util::Renderer& root) {
    for (auto& b : m_Bombs) root.RemoveChild(b);
    for (auto& e : m_Explosions) root.RemoveChild(e);
    m_Bombs.clear();
    m_Explosions.clear();
}

bool BombManager::IsBombAt(int gridX, int gridY, const Bomb* ignore) const {
    for (const auto& b : m_Bombs) {
        if (b.get() == ignore) continue; // Skip self when used by Bomb::Update
        if (b->GetGridX() == gridX && b->GetGridY() == gridY) return true;
    }
    return false;
}

bool BombManager::HasExplosionAt(int gridX, int gridY) const {
    for (const auto& e : m_Explosions) {
        if (e->GetGridX() == gridX && e->GetGridY() == gridY) return true;
    }
    return false;
}

int BombManager::GetFirepowerAt(int gridX, int gridY) const {
    for (const auto& b : m_Bombs) {
        if (b->GetGridX() == gridX && b->GetGridY() == gridY) return b->GetFirepower();
    }
    return 0; // No bomb here, firepower is 0
}

void BombManager::SpawnBomb(int gridX, int gridY, int firepower, int ownerID, Util::Renderer& root) {
    auto newBomb = std::make_shared<Bomb>(gridX, gridY, firepower, ownerID);
    m_Bombs.push_back(newBomb);
    root.AddChild(newBomb);
}