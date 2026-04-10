#include "BombManager.hpp"
#include "InteractableManager.hpp"
#include "Player.hpp"
#include "Util/Logger.hpp"

// 放置炸彈
void BombManager::PlaceBomb(std::shared_ptr<Player>& player, LevelManager& levelManager, InteractableManager& interactableManager, Util::Renderer& root, const std::vector<std::shared_ptr<Player>>& players) {
    if (!player->CanPlaceBomb())  // 玩家不能放炸彈
        return;

    // 計算玩家座標
    int targetX = player->GetGridX();
    int targetY = player->GetGridY();

    float bombPixelX = (targetX - 12) * 32.0f;
    float bombPixelY = (8 - targetY) * 32.0f;

    for (const auto& p : players) {
        if (p->GetPlayerID() == player->GetPlayerID() || p->IsDead()) continue;
        if (std::abs(p->GetPixelPos().x - bombPixelX) < 32.0f && std::abs(p->GetPixelPos().y - bombPixelY) < 32.0f) {
            LOG_INFO("Cannot place bomb: Another player is occupying the grid.");
            return;
        }
    }

    // 檢查目標位置是否是草地且沒有其他炸彈
    if (levelManager.IsWalkable(targetX, targetY) && !IsBombAt(targetX, targetY) && !interactableManager.IsInteractableAt(targetX, targetY)) {
        auto newBomb = std::make_shared<Bomb>(targetX, targetY, player->GetFirepower(), player->GetPlayerID());
        m_Bombs.push_back(newBomb);
        root.AddChild(newBomb);

        player->AddBombCount(); // 增加計數

        for (const auto& p : players) {  // 放置後暫時忽略這格子，避免卡住
            if (p->IsDead()) continue;
            if (std::abs(p->GetPixelPos().x - bombPixelX) < 40.0f && std::abs(p->GetPixelPos().y - bombPixelY) < 40.0f) {
                p->SetIgnoreBomb(targetX, targetY);
            }
        }

        LOG_INFO("Bomb placed at (" + std::to_string(targetX) + ", " + std::to_string(targetY) + ")");
    }
}

// 更新炸彈與火焰
void BombManager::Update(LevelManager& levelManager, InteractableManager& interactableManager, Util::Renderer& root, std::vector<std::shared_ptr<Player>>& players) {
    for (auto it = m_Bombs.begin(); it != m_Bombs.end(); ) {  // 炸彈生命週期與火焰延伸
        (*it)->Update();

        if ((*it)->GetState() == Bomb::State::DONE) {
            int bx = (*it)->GetGridX();
            int by = (*it)->GetGridY();
            int fp = (*it)->GetFirepower();

            // 中心點火焰
            auto centerFire = std::make_shared<Explosion>(bx, by);
            m_Explosions.push_back(centerFire);
            root.AddChild(centerFire);

            // 火焰延伸
            int dx[] = { 0, 0, -1, 1 };
            int dy[] = { -1, 1, 0, 0 };

            for (int dir = 0; dir < 4; dir++) {
                for (int step = 1; step <= fp; step++) {
                    int targetX = bx + dx[dir] * step;
                    int targetY = by + dy[dir] * step;

                    if (!levelManager.IsWalkable(targetX, targetY)) {
                        if (levelManager.IsBrick(targetX, targetY)) {
                            // 在磚塊上引爆一團火焰
                            auto fire = std::make_shared<Explosion>(targetX, targetY);
                            m_Explosions.push_back(fire);
                            root.AddChild(fire);

                            // 破壞磚塊
                            levelManager.DestroyBrick(targetX, targetY, root, interactableManager);

                        }

                        break;
                    }

                    // 物件燒毀判定 (如果放在生物那會造成剛生成就銷毀)
                    auto& interactables = interactableManager.GetInteractables();
                    for (auto itI = interactables.begin(); itI != interactables.end(); ) {
                        if ((*itI)->GetGridX() == targetX && (*itI)->GetGridY() == targetY && (*itI)->IsDestroyedByFire()) {
                            interactableManager.RemoveItem(itI, root);
                        }
                        else { ++itI; }
                    }

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

    // 傷害判定
    for (auto it = m_Explosions.begin(); it != m_Explosions.end(); ) {
        (*it)->Update();

        // 生物燒毀判定
        for (auto& player : players) {
            if (!player->IsDead() &&
                player->GetGridX() == (*it)->GetGridX() &&
                player->GetGridY() == (*it)->GetGridY()) {

                player->Kill();
                LOG_INFO("Player was BURNED!");
            }
        }

		// 連鎖爆炸
        for (auto& bomb : m_Bombs) {
            if (bomb->GetState() == Bomb::State::COUNTDOWN &&
                bomb->GetGridX() == (*it)->GetGridX() &&
                bomb->GetGridY() == (*it)->GetGridY()) {

                bomb->ForceDetonate(); // 強制提早引爆
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

bool BombManager::IsBombAt(int gridX, int gridY) const {
    for (const auto& b : m_Bombs) {
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
    return 0; // 沒炸彈火力為 0
}