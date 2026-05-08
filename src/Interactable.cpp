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

// 輸送帶
Conveyor::Conveyor(int gridX, int gridY, Player::Direction dir) : m_GridX(gridX), m_GridY(gridY), m_Dir(dir) {
    std::string imgPath;
    switch (dir) {
        case Player::Direction::UP: 
            imgPath = RESOURCE_DIR"/Image/conveyor_up.png"; 
            break;
        case Player::Direction::DOWN: 
            imgPath = RESOURCE_DIR"/Image/conveyor_down.png"; 
            break;
        case Player::Direction::LEFT: 
            imgPath = RESOURCE_DIR"/Image/conveyor_left.png"; 
            break;
        case Player::Direction::RIGHT: 
            imgPath = RESOURCE_DIR"/Image/conveyor_right.png"; 
            break;
    }

    auto image = std::make_shared<Util::Image>(imgPath);
    SetDrawable(image);
    SetZIndex(2);  // 4 (炸彈) > 2 > 1 (草地)

    m_Transform.scale = { 32.0f / image->GetSize().x, 32.0f / image->GetSize().y };
    m_Transform.translation = { (gridX - 12) * 32.0f, (8 - gridY) * 32.0f };
}

glm::vec2 Conveyor::GetForce() const {
    float pushSpeed = 1.5f;

    switch (m_Dir) {
        case Player::Direction::UP: 
            return { 0.0f, pushSpeed };
        case Player::Direction::DOWN: 
            return { 0.0f, -pushSpeed };
        case Player::Direction::LEFT: 
            return { -pushSpeed, 0.0f };
        case Player::Direction::RIGHT: 
            return { pushSpeed, 0.0f };
    }
    return { 0.0f, 0.0f };
}

// 彈跳板
BouncePad::BouncePad(int gridX, int gridY, Player::Direction dir)
    : m_GridX(gridX), m_GridY(gridY), m_Dir(dir) {

    std::string imgPath;
    switch (dir) {
    case Player::Direction::UP:
        imgPath = RESOURCE_DIR"/Image/bouncepad_up.png";
        break;
    case Player::Direction::DOWN:
        imgPath = RESOURCE_DIR"/Image/bouncepad_down.png";
        break;
    case Player::Direction::LEFT:
        imgPath = RESOURCE_DIR"/Image/bouncepad_left.png";
        break;
    case Player::Direction::RIGHT:
        imgPath = RESOURCE_DIR"/Image/bouncepad_right.png";
        break;
    }

    auto image = std::make_shared<Util::Image>(imgPath);
    SetDrawable(image);
    SetZIndex(2);

    m_Transform.scale = { 32.0f / image->GetSize().x, 32.0f / image->GetSize().y };
    m_Transform.translation = { (gridX - 12) * 32.0f, (8 - gridY) * 32.0f };
}

void BouncePad::Update() {
    if (m_Cooldown > 0) {
        m_Cooldown--;
    }
}

bool BouncePad::OnInteract(std::shared_ptr<Player>& player) {
    if (m_Cooldown > 0) return false;

    if (player->TriggerBounce(m_Dir, m_Distance)) {
        m_Cooldown = 60 * 5;  // 5 秒冷卻
        LOG_INFO("BouncePad triggered! Cooldown started.");
    }

    return false;
}