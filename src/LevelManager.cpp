#include "LevelManager.hpp"
#include "MapTiles.hpp"
#include <fstream>
#include "Util/Logger.hpp"

void LevelManager::LoadLevel(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        LOG_ERROR("Error opening file: " + filepath);
        return;
    }

    // m_MapData.assign(17, std::vector<char>(25, '0'));
    m_TileGrid.assign(17, std::vector<std::shared_ptr<Tile>>(25));

    m_Tiles.clear();
    for (int y=0;y<17;y++) {
        for (int x=0;x<25;x++) {
            char type; 
            file >> type;

            // m_MapData[y][x] = type;  // 存地圖方塊
            std::shared_ptr<Tile> tile;  // 這個地圖方塊
            
            //m_Tiles.push_back(std::make_shared<Ground>(x, y));  // 鋪設草地
            //
            //if (type == '1') {  // 鋪設無敵牆
            //    m_Tiles.push_back(std::make_shared<Wall>(x, y));
            //}
            //else if (type == '2') {  // 鋪設磚塊
            //    m_Tiles.push_back(std::make_shared<Brick>(x, y));
            //}

            if (type == '1') {  // 鋪設無敵牆
                tile = std::make_shared<Wall>(x, y);
            }
            else if (type == '2') {   // 鋪設磚塊
                tile = std::make_shared<Brick>(x, y);
            }
            else {
                tile = std::make_shared<Ground>(x, y);
            }

            m_TileGrid[y][x] = tile;  // 存地圖方塊
            m_Tiles.push_back(tile);  // 鋪設方塊
        }
    }
    LOG_INFO("Map loading complete, total: " + std::to_string(m_Tiles.size()) + " objects");
}

void LevelManager::AttachToRoot(Util::Renderer& root) {
    for (auto& tile : m_Tiles) {
        root.AddChild(tile);
    }
}

void LevelManager::DetachFromRoot(Util::Renderer& root) {
    for (auto& tile : m_Tiles) {
        root.RemoveChild(tile);
    }
}

// 碰撞查詢
bool LevelManager::IsWalkable(int gridX, int gridY) const {
    if (gridX < 0 || gridX >= 25 || gridY < 0 || gridY >= 17)
        return false;

    //char type = m_MapData[gridY][gridX];

    // 可走的回傳
    return m_TileGrid[gridY][gridX]->IsPassable();
}

// 檢查該格是否為磚塊
bool LevelManager::IsBrick(int gridX, int gridY) const {
    if (gridX < 0 || gridX >= 25 || gridY < 0 || gridY >= 17) 
        return false;

    // 檢查 m_TileGrid 裡面那個指標是不是 Brick 型態
    return std::dynamic_pointer_cast<Brick>(m_TileGrid[gridY][gridX]) != nullptr;
}

// 摧毀磚塊並長出草地
void LevelManager::DestroyBrick(int gridX, int gridY, Util::Renderer& root) {
    if (!IsBrick(gridX, gridY)) return;

    auto oldBrick = m_TileGrid[gridY][gridX];  // 待摧毀的方塊

    root.RemoveChild(oldBrick);  // 移除畫面

    // 從 m_Tiles 一維陣列中清除 (避免記憶體洩漏)
    for (auto it = m_Tiles.begin(); it != m_Tiles.end(); ++it) {
        if (*it == oldBrick) {
            m_Tiles.erase(it);
            break;
        }
    }

    // 在原地生成一塊新的草地，補上破洞
    auto newGround = std::make_shared<Ground>(gridX, gridY);
    m_TileGrid[gridY][gridX] = newGround; // 更新二維陣列
    m_Tiles.push_back(newGround);         // 加入實體清單
    root.AddChild(newGround);             // 畫到螢幕上
}