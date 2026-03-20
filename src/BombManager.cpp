#include "BombManager.hpp"
#include "Player.hpp"
#include "Util/Logger.hpp"

// 放置炸彈
void BombManager::PlaceBomb(std::shared_ptr<Player>& player, LevelManager& levelManager, InteractableManager& interactableManager, Util::Renderer& root) {
    if (!player->CanPlaceBomb())  // 玩家不能放炸彈
        return;

    // 計算玩家方向
    int targetX = player->GetGridX();
    int targetY = player->GetGridY();

    // 取得玩家當前方向
    switch (player->GetDirection()) {
        case Player::Direction::UP:    
            targetY--; 
            break;
        case Player::Direction::DOWN:  
            targetY++; 
            break;
        case Player::Direction::LEFT:  
            targetX--; 
            break;
        case Player::Direction::RIGHT: 
            targetX++; 
            break;
    }

    // 檢查目標位置是否是草地且沒有其他炸彈
    if (levelManager.IsWalkable(targetX, targetY) && !IsBombAt(targetX, targetY) && !interactableManager.IsInteractableAt(targetX, targetY)) {
        auto newBomb = std::make_shared<Bomb>(targetX, targetY, 2); // 火力固定 2
        m_Bombs.push_back(newBomb);
        root.AddChild(newBomb);

        player->AddBombCount(); // 增加計數

        player->SetIgnoreBomb(targetX, targetY);  // 放置後暫時忽略這格子，避免卡住

        LOG_INFO("Bomb placed in front at (" + std::to_string(targetX) + ", " + std::to_string(targetY) + ")");
    }
}

// 更新炸彈與火焰
void BombManager::Update(LevelManager& levelManager, Util::Renderer& root, std::shared_ptr<Player>& player) {
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

            //for (int dir = 0; dir < 4; dir++) {
            //    for (int step = 1; step <= fp; step++) {
            //        int targetX = bx + dx[dir] * step;
            //        int targetY = by + dy[dir] * step;

            //        if (!levelManager.IsWalkable(targetX, targetY)) {
            //            break; // 撞牆停止蔓延
            //        }

            //        auto fire = std::make_shared<Explosion>(targetX, targetY);
            //        m_Explosions.push_back(fire);
            //        root.AddChild(fire);
            //    }
            //}

            for (int dir = 0; dir < 4; dir++) {
                for (int step = 1; step <= fp; step++) {
                    int targetX = bx + dx[dir] * step;
                    int targetY = by + dy[dir] * step;

                    // 如果撞到不可行走的東西 (無敵牆 或 磚塊)
                    if (!levelManager.IsWalkable(targetX, targetY)) {

						// 詢問是否是磚塊，如果是的話要先引爆火焰再破壞磚塊
                        if (levelManager.IsBrick(targetX, targetY)) {
                            // 在磚塊上引爆一團火焰
                            auto fire = std::make_shared<Explosion>(targetX, targetY);
                            m_Explosions.push_back(fire);
                            root.AddChild(fire);

                            // 破壞磚塊
                            levelManager.DestroyBrick(targetX, targetY, root);
                        }

                        // 停止延伸火焰
                        break;
                    }

                    // 如果是普通空地，正常延伸火焰
                    auto fire = std::make_shared<Explosion>(targetX, targetY);
                    m_Explosions.push_back(fire);
                    root.AddChild(fire);
                }
            }

			player->DecBombCount();  // 減少計數
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
        if (!player->IsDead() &&
            player->GetGridX() == (*it)->GetGridX() &&
            player->GetGridY() == (*it)->GetGridY()) {

            player->Kill();
            LOG_INFO("Player was BURNED!");
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