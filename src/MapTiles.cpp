#include "MapTiles.hpp"
#include "Util/Image.hpp"

Ground::Ground(int gridX, int gridY) {
    auto image = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/ground.png");
    SetDrawable(image);
    SetZIndex(1); 
    
    m_Transform.scale = { 32.0f / image->GetSize().x, 32.0f / image->GetSize().y };  // 限制大小
    m_Transform.translation = { (gridX - 12) * 32.0f, (8 - gridY) * 32.0f };
}

Wall::Wall(int gridX, int gridY) {
    auto image = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/wall.png");
    SetDrawable(image);
    SetZIndex(5); 

    m_Transform.scale = { 32.0f / image->GetSize().x, 32.0f / image->GetSize().y };  // 限制大小
    m_Transform.translation = { (gridX - 12) * 32.0f, (8 - gridY) * 32.0f };
}

Brick::Brick(int gridX, int gridY) {
    auto image = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/brick.png");
    SetDrawable(image);
    SetZIndex(5); 

    m_Transform.scale = { 32.0f / image->GetSize().x, 32.0f / image->GetSize().y };  // 限制大小
    m_Transform.translation = { (gridX - 12) * 32.0f, (8 - gridY) * 32.0f };
}