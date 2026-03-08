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

    m_MapData.assign(17, std::vector<char>(25, '0'));

    m_Tiles.clear();
    for (int y=0;y<17;y++) {
        for (int x=0;x<25;x++) {
            char type; 
            file >> type;

            m_MapData[y][x] = type;  // 存地圖方塊
            
            m_Tiles.push_back(std::make_shared<Ground>(x, y));  // 鋪設草地
            
            if (type == '1') {  // 鋪設無敵牆
                m_Tiles.push_back(std::make_shared<Wall>(x, y));
            }
            else if (type == '2') {  // 鋪設磚塊
                m_Tiles.push_back(std::make_shared<Brick>(x, y));
            }
        }
    }
    LOG_INFO("Map loading complete, total: " + std::to_string(m_Tiles.size()) + " objects");
}

void LevelManager::AttachToRoot(Util::Renderer& root) {
    for (auto& tile : m_Tiles) {
        root.AddChild(tile);
    }
}

// 碰撞查詢函式
bool LevelManager::IsWalkable(int gridX, int gridY) const {
    if (gridX < 0 || gridX >= 25 || gridY < 0 || gridY >= 17)
        return false;

    char type = m_MapData[gridY][gridX];

    // 排除不能走的：牆1、磚塊2、砲台 B
    return type != '1' && type != '2' && type != 'B';
}