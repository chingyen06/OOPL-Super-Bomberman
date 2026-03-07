#include "MapTiles.hpp"
#include "Util/Image.hpp"

Ground::Ground(int gridX, int gridY) {
    auto image = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/ground.png");
    SetDrawable(image);
    SetZIndex(1); 
    
    m_Transform.scale = { 48.0f / image->GetSize().x, 48.0f / image->GetSize().y };  // ����j�p
    m_Transform.translation = { (gridX - 7) * 48.0f, (6 - gridY) * 48.0f };
}

Wall::Wall(int gridX, int gridY) {
    auto image = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/wall.png");
    SetDrawable(image);
    SetZIndex(5); 

    m_Transform.scale = { 48.0f / image->GetSize().x, 48.0f / image->GetSize().y };  // ����j�p
    m_Transform.translation = { (gridX - 7) * 48.0f, (6 - gridY) * 48.0f };
}

Brick::Brick(int gridX, int gridY) {
    auto image = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/brick.png");
    SetDrawable(image);
    SetZIndex(5); 

    m_Transform.scale = { 48.0f / image->GetSize().x, 48.0f / image->GetSize().y };  // ����j�p
    m_Transform.translation = { (gridX - 7) * 48.0f, (6 - gridY) * 48.0f };
}