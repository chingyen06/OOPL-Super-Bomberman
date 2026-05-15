#include "Turret/Projectile.hpp"
#include "LevelManager.hpp"
#include "BombManager.hpp"
#include "Util/Logger.hpp"
#include <cmath>

Projectile::Projectile(int startGridX, int startGridY, int targetGridX, int targetGridY, Player::Direction dir)
    : m_Dir(dir), m_TargetGridX(targetGridX), m_TargetGridY(targetGridY) {

    auto img = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/bomb.png");
    SetDrawable(img);
    SetZIndex(15);

    m_Transform.scale = { (32.0f / img->GetSize().x) * 0.6f, (32.0f / img->GetSize().y) * 0.6f };

    m_Pos.x = (startGridX - 12) * 32.0f;
    m_Pos.y = (8 - startGridY) * 32.0f;
    m_Transform.translation = m_Pos;

    m_TargetPixelX = (targetGridX - 12) * 32.0f;
    m_TargetPixelY = (8 - targetGridY) * 32.0f;
}

void Projectile::Update(std::vector<std::shared_ptr<Player>>& players, const LevelManager& lm, BombManager& bm, Util::Renderer& root) {
    if (m_IsDead) return;

    float dx = m_TargetPixelX - m_Pos.x;
    float dy = m_TargetPixelY - m_Pos.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    if (dist <= m_Speed) {
        m_Pos.x = m_TargetPixelX;
        m_Pos.y = m_TargetPixelY;
        m_Transform.translation = m_Pos;
        m_IsDead = true;

        if (!bm.IsBombAt(m_TargetGridX, m_TargetGridY)) {
            bm.SpawnBomb(m_TargetGridX, m_TargetGridY, 2, -1, root); // ¹w³]¤õ¤O 2
            LOG_INFO("Turret Bomb Landed!");
        }
    }
    else {
        m_Pos.x += (dx / dist) * m_Speed;
        m_Pos.y += (dy / dist) * m_Speed;
        m_Transform.translation = m_Pos;
    }
}