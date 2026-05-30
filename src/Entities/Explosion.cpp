#include "Explosion.hpp"
#include "GameConstants.hpp"
#include "GridCoord.hpp"

Explosion::Explosion(int gridX, int gridY) : m_GridX(gridX), m_GridY(gridY), m_Tick(Constants::Bomb::kExplosionFrames), m_Done(false) {
    auto image = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/explosion.png");
    SetDrawable(image);
    SetZIndex(10);

    m_Transform.scale = { GridCoord::kTileSize / image->GetSize().x, GridCoord::kTileSize / image->GetSize().y };
    m_Transform.translation = GridCoord::ToPixel(gridX, gridY);
}

void Explosion::Update() {
    if (!m_Done) {
        m_Tick--;
        if (m_Tick <= 0) 
            m_Done = true;
    }
}