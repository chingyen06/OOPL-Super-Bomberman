#include "Explosion.hpp"

Explosion::Explosion(int gridX, int gridY) : m_GridX(gridX), m_GridY(gridY), m_Tick(30), m_Done(false) {
    auto image = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/explosion.png");
    SetDrawable(image);
    SetZIndex(10);

    m_Transform.scale = { 32.0f / image->GetSize().x, 32.0f / image->GetSize().y };
    m_Transform.translation = { (gridX - 12) * 32.0f, (8 - gridY) * 32.0f };
}

void Explosion::Update() {
    if (!m_Done) {
        m_Tick--;
        if (m_Tick <= 0) 
            m_Done = true;
    }
}