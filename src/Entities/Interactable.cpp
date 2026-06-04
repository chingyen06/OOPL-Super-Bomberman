#include "Interactable.hpp"
#include "Player.hpp"
#include "GameConstants.hpp"
#include "GridCoord.hpp"
#include "Util/Image.hpp"
#include "Util/Logger.hpp"

Key::Key(int gridX, int gridY) : Interactable(gridX, gridY) {
    auto image = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/key.png");
    SetDrawable(image);
    SetZIndex(6);

    m_Transform.scale = { GridCoord::kTileSize / image->GetSize().x, GridCoord::kTileSize / image->GetSize().y };
    m_Transform.translation = GridCoord::ToPixel(gridX, gridY);
}

bool Key::OnInteract(Player& player) {
    if (player.GetTeam() != Team::ATTACKER)
        return false;

    if (!player.HasKey()) {
        player.SetKey(true);
        LOG_INFO("Player picked up the Key!");
        return true;
    }
    return false;
}

Chest::Chest(int gridX, int gridY) : Interactable(gridX, gridY) {
    m_ClosedImage = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/chest_closed.png");
    m_OpenedImage = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/chest_opened.png");

    SetDrawable(m_ClosedImage);
    SetZIndex(5);

    m_Transform.scale = { GridCoord::kTileSize / m_ClosedImage->GetSize().x, GridCoord::kTileSize / m_ClosedImage->GetSize().y };
    m_Transform.translation = GridCoord::ToPixel(gridX, gridY);
}

void Chest::Open() {
    if (m_Opened) 
        return;

    m_Opened = true;
    SetDrawable(m_OpenedImage);
    LOG_INFO("Chest opened at (" + std::to_string(m_GridX) + ", " + std::to_string(m_GridY) + ")");
}

bool Chest::OnInteract(Player& player) {
    if (player.GetTeam() != Team::ATTACKER)
        return false;

    if (!m_Opened && player.HasKey()) {
        Open();
        player.SetKey(false);
        LOG_INFO("Chest Opened! Check victory condition here.");
    }
    return false;
}


PowerUp::PowerUp(int gridX, int gridY, std::unique_ptr<IPlayerEffect> effect, const std::string& imagePath)
    : Interactable(gridX, gridY), m_Effect(std::move(effect)) {
    auto image = std::make_shared<Util::Image>(imagePath);
    SetDrawable(image);
    SetZIndex(6);
    m_Transform.scale = { GridCoord::kTileSize / image->GetSize().x, GridCoord::kTileSize / image->GetSize().y };
    m_Transform.translation = GridCoord::ToPixel(gridX, gridY);
}

bool PowerUp::OnInteract(Player& player) {
    if (!m_Effect) return false;
    m_Effect->Apply(player);
    LOG_INFO(std::string("Player picked up ") + m_Effect->GetLogName() + "!");
    return true;
}

// Conveyor
Conveyor::Conveyor(int gridX, int gridY, Direction dir) : Interactable(gridX, gridY), m_Dir(dir) {
    std::string imgPath;
    switch (dir) {
        case Direction::UP: 
            imgPath = RESOURCE_DIR"/Image/conveyor_up.png"; 
            break;
        case Direction::DOWN: 
            imgPath = RESOURCE_DIR"/Image/conveyor_down.png"; 
            break;
        case Direction::LEFT: 
            imgPath = RESOURCE_DIR"/Image/conveyor_left.png"; 
            break;
        case Direction::RIGHT: 
            imgPath = RESOURCE_DIR"/Image/conveyor_right.png"; 
            break;
    }

    auto image = std::make_shared<Util::Image>(imgPath);
    SetDrawable(image);
    SetZIndex(2);  // 4 (炸彈) > 2 > 1 (地板)

    m_Transform.scale = { GridCoord::kTileSize / image->GetSize().x, GridCoord::kTileSize / image->GetSize().y };
    m_Transform.translation = GridCoord::ToPixel(gridX, gridY);
}

glm::vec2 Conveyor::GetForce() const {
    constexpr float pushSpeed = Constants::Conveyor::kPushSpeed;

    switch (m_Dir) {
        case Direction::UP: 
            return { 0.0f, pushSpeed };
        case Direction::DOWN: 
            return { 0.0f, -pushSpeed };
        case Direction::LEFT: 
            return { -pushSpeed, 0.0f };
        case Direction::RIGHT: 
            return { pushSpeed, 0.0f };
    }
    return { 0.0f, 0.0f };
}

// BouncePad
BouncePad::BouncePad(int gridX, int gridY, Direction dir)
    : Interactable(gridX, gridY), m_Dir(dir) {

    std::string activePath;
    std::string inactivePath;

    switch (dir) {
        case Direction::UP:
            activePath = RESOURCE_DIR"/Image/bouncepad_up.png";
            inactivePath = RESOURCE_DIR"/Image/bouncepad_up_gray.png";
            break;
        case Direction::DOWN:
            activePath = RESOURCE_DIR"/Image/bouncepad_down.png";
            inactivePath = RESOURCE_DIR"/Image/bouncepad_down_gray.png";
            break;
        case Direction::LEFT:
            activePath = RESOURCE_DIR"/Image/bouncepad_left.png";
            inactivePath = RESOURCE_DIR"/Image/bouncepad_left_gray.png";
            break;
        case Direction::RIGHT:
            activePath = RESOURCE_DIR"/Image/bouncepad_right.png";
            inactivePath = RESOURCE_DIR"/Image/bouncepad_right_gray.png";
            break;
    }

    m_ActiveImage = std::make_shared<Util::Image>(activePath);
    m_InactiveImage = std::make_shared<Util::Image>(inactivePath);

    SetDrawable(m_ActiveImage);
    SetZIndex(2);

    m_Transform.scale = { GridCoord::kTileSize / m_ActiveImage->GetSize().x, GridCoord::kTileSize / m_ActiveImage->GetSize().y };
    m_Transform.translation = GridCoord::ToPixel(gridX, gridY);
}

void BouncePad::Update() {
    if (m_Cooldown > 0) {
        m_Cooldown--;

        if (m_Cooldown == 0) {
            SetDrawable(m_ActiveImage);
        }
    }
}

bool BouncePad::OnInteract(Player& player) {
    if (m_Cooldown > 0) return false;

    if (player.TriggerBounce(m_Dir, m_Distance)) {
        m_Cooldown = Constants::BouncePad::kCooldownFrames;
        SetDrawable(m_InactiveImage);
        LOG_INFO("BouncePad triggered! Cooldown started.");
    }

    return false;
}
