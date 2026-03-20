#include "Interactable.hpp"
#include "Util/Image.hpp"
#include "Util/Logger.hpp"

Key::Key(int gridX, int gridY) : m_GridX(gridX), m_GridY(gridY) {
    auto image = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/key.png");
    SetDrawable(image);
    SetZIndex(6);

    m_Transform.scale = { 32.0f / image->GetSize().x, 32.0f / image->GetSize().y };
    m_Transform.translation = { (gridX - 12) * 32.0f, (8 - gridY) * 32.0f };
}

Chest::Chest(int gridX, int gridY) : m_GridX(gridX), m_GridY(gridY) {
    m_ClosedImage = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/chest_closed.png");
    m_OpenedImage = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/chest_opened.png");

    SetDrawable(m_ClosedImage);
    SetZIndex(5);

    m_Transform.scale = { 32.0f / m_ClosedImage->GetSize().x, 32.0f / m_ClosedImage->GetSize().y };
    m_Transform.translation = { (gridX - 12) * 32.0f, (8 - gridY) * 32.0f };
}

void Chest::Open() {
    if (m_Opened) 
        return;

    m_Opened = true;
    SetDrawable(m_OpenedImage);
    LOG_INFO("Chest opened at (" + std::to_string(m_GridX) + ", " + std::to_string(m_GridY) + ")");
}