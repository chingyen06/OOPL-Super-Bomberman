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

    m_Tiles.clear();
    for (int y=0;y<13;y++) {
        for (int x=0;x<15;x++) {
            int type; 
            file >> type;
            
            m_Tiles.push_back(std::make_shared<Ground>(x, y));  // 鋪設草地
            
            if (type == 1) {  // 鋪設無敵牆
                m_Tiles.push_back(std::make_shared<Wall>(x, y));
            }
            else if (type == 2) {  // 鋪設磚塊
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