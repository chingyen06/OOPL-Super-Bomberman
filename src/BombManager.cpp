#include "BombManager.hpp"
#include "Util/Logger.hpp"

// 放置炸彈
void BombManager::PlaceBomb(int gridX, int gridY, int firepower, Util::Renderer& root) {
    for (const auto& b : m_Bombs) {
        if (b->GetGridX() == gridX && b->GetGridY() == gridY) 
            return; // 防止同格重複放置
    }

    auto newBomb = std::make_shared<Bomb>(gridX, gridY, firepower);
    m_Bombs.push_back(newBomb);
    root.AddChild(newBomb);
    LOG_INFO("Bomb placed at (" + std::to_string(gridX) + ", " + std::to_string(gridY) + ")");
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