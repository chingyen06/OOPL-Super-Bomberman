#ifndef LEVELMANAGER_HPP
#define LEVELMANAGER_HPP

#include <vector>
#include <string>
#include <memory>
#include "Util/GameObject.hpp"
#include "Util/Renderer.hpp"

class LevelManager {
public:
    void LoadLevel(const std::string& filepath);
    void AttachToRoot(Util::Renderer& root);
    bool IsWalkable(int gridX, int gridY) const;  // 查詢是否可移動至 (gridX, gridY)
    
private:
    std::vector<std::shared_ptr<Util::GameObject>> m_Tiles;
    std::vector<std::vector<char>> m_MapData;  // 存地圖方塊
};

#endif