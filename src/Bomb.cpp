#include "Bomb.hpp"

Bomb::Bomb(int gridX, int gridY, int firepower) : m_GridX(gridX), m_GridY(gridY), m_Firepower(firepower), m_Tick(180), m_State(State::COUNTDOWN) {
    auto image = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/bomb.png");
    SetDrawable(image);
    SetZIndex(4); // 草地上，主角與牆壁下方

    m_Transform.scale = { 32.0f / image->GetSize().x, 32.0f / image->GetSize().y };
    m_Transform.translation = { (gridX - 12) * 32.0f, (8 - gridY) * 32.0f };
}

void Bomb::Update() {
    if (m_State == State::COUNTDOWN) {
        m_Tick--;
        if (m_Tick <= 0) 
            m_State = State::DONE;
    }
}