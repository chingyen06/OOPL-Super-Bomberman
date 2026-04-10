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

bool Key::OnInteract(std::shared_ptr<Player>& player) {
    if (player->GetTeam() != Team::ATTACKER)
        return false;

    if (!player->HasKey()) {
        player->SetKey(true);
        LOG_INFO("Player picked up the Key!");
        return true;
    }
    return false;
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

bool Chest::OnInteract(std::shared_ptr<Player>& player) {
    if (player->GetTeam() != Team::ATTACKER)
        return false;

    if (!m_Opened && player->HasKey()) {
        Open();
        player->SetKey(false);
        LOG_INFO("Chest Opened! Check victory condition here.");
    }
    return false;
}


PowerUp::PowerUp(int gridX, int gridY) : m_GridX(gridX), m_GridY(gridY) {}


SpeedItem::SpeedItem(int gridX, int gridY) : PowerUp(gridX, gridY) {
    auto image = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/item_speedup.png");
    SetDrawable(image);
    SetZIndex(6);
    m_Transform.scale = { 32.0f / image->GetSize().x, 32.0f / image->GetSize().y };
    m_Transform.translation = { (gridX - 12) * 32.0f, (8 - gridY) * 32.0f };
}

bool SpeedItem::OnInteract(std::shared_ptr<Player>& player) {
    player->ActivateSpeedBoost(); // 啟動 5 秒計時器
    LOG_INFO("Player picked up SPEED_UP! (Temporary 5s boost)");
    return true;
}

BombItem::BombItem(int gridX, int gridY) : PowerUp(gridX, gridY) {
    auto image = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/item_bombup.png");
    SetDrawable(image);
    SetZIndex(6);
    m_Transform.scale = { 32.0f / image->GetSize().x, 32.0f / image->GetSize().y };
    m_Transform.translation = { (gridX - 12) * 32.0f, (8 - gridY) * 32.0f };
}

bool BombItem::OnInteract(std::shared_ptr<Player>& player) {
    player->IncreaseMaxBombs();
    LOG_INFO("Player picked up BOMB_UP! Max bombs increased to " + std::to_string(player->CanPlaceBomb()));
    return true;
}

FireItem::FireItem(int gridX, int gridY) : PowerUp(gridX, gridY) {
    auto image = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/item_fireup.png");
    SetDrawable(image);
    SetZIndex(6);
    m_Transform.scale = { 32.0f / image->GetSize().x, 32.0f / image->GetSize().y };
    m_Transform.translation = { (gridX - 12) * 32.0f, (8 - gridY) * 32.0f };
}

bool FireItem::OnInteract(std::shared_ptr<Player>& player) {
    player->IncreaseFirepower();
    LOG_INFO("Player picked up FIRE_UP! Firepower increased to " + std::to_string(player->GetFirepower()));
    return true;
}