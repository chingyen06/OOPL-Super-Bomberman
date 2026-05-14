#include "Turret/Projectile.hpp"
#include "LevelManager.hpp"
#include "BombManager.hpp"
#include "Util/Logger.hpp"

Projectile::Projectile(int startGridX, int startGridY, Player::Direction dir)
    : m_Dir(dir) {
    auto img = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/bomb.png");
    SetDrawable(img);
    SetZIndex(10);

    m_Pos.x = (startGridX - 12) * 32.0f;
    m_Pos.y = (8 - startGridY) * 32.0f;
    m_Transform.translation = m_Pos;
}

void Projectile::Update(std::vector<std::shared_ptr<Player>>& players, const LevelManager& lm, const BombManager& bm) {
    if (m_IsDead) return;

    // ¦ì²¾­pºâ
    float dx = 0.0f, dy = 0.0f;
    if (m_Dir == Player::Direction::UP) dy = m_Speed;
    else if (m_Dir == Player::Direction::DOWN) dy = -m_Speed;
    else if (m_Dir == Player::Direction::LEFT) dx = -m_Speed;
    else if (m_Dir == Player::Direction::RIGHT) dx = m_Speed;

    m_Pos.x += dx;
    m_Pos.y += dy;
    m_Transform.translation = m_Pos;

    // ¸I¼²°»´ú
    CheckCollision(players, lm, bm);
}

void Projectile::CheckCollision(std::vector<std::shared_ptr<Player>>& players, const LevelManager& lm, const BombManager& bm) {
    int currentGridX = std::round(m_Pos.x / 32.0f) + 12;
    int currentGridY = 8 - std::round(m_Pos.y / 32.0f);

    // ¸I¨ìÀð¾À¡B¿j¶ô©Î¬µ¼u§Y¾P·´
    if (!lm.IsWalkable(currentGridX, currentGridY) || bm.IsBombAt(currentGridX, currentGridY)) {
        m_IsDead = true;
        return;
    }

    // À»¤¤ª±®a
    for (auto& p : players) {
        if (!p->IsDead() && p->GetGridX() == currentGridX && p->GetGridY() == currentGridY) {
            p->Kill();
            LOG_INFO("Player killed by Turret Projectile!");
            m_IsDead = true;
            return;
        }
    }
}