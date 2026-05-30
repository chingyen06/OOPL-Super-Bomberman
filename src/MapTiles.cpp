#include "MapTiles.hpp"
#include "GridCoord.hpp"
#include "Util/Image.hpp"

Ground::Ground(int gridX, int gridY, const std::string& imagePath) {
    auto image = std::make_shared<Util::Image>(imagePath);
    SetDrawable(image);
    SetZIndex(1);

    m_Transform.scale = { GridCoord::kTileSize / image->GetSize().x, GridCoord::kTileSize / image->GetSize().y };
    m_Transform.translation = GridCoord::ToPixel(gridX, gridY);
}

Wall::Wall(int gridX, int gridY, const std::string& imagePath) {
    auto image = std::make_shared<Util::Image>(imagePath);
    SetDrawable(image);
    SetZIndex(15);

    m_Transform.scale = { GridCoord::kTileSize / image->GetSize().x, GridCoord::kTileSize / image->GetSize().y };
    m_Transform.translation = GridCoord::ToPixel(gridX, gridY);
}

Brick::Brick(int gridX, int gridY, const std::string& imagePath) {
    auto image = std::make_shared<Util::Image>(imagePath);
    SetDrawable(image);
    SetZIndex(5);

    m_Transform.scale = { GridCoord::kTileSize / image->GetSize().x, GridCoord::kTileSize / image->GetSize().y };
    m_Transform.translation = GridCoord::ToPixel(gridX, gridY);
}
