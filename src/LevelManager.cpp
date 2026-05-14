#include "LevelManager.hpp"
#include "InteractableManager.hpp"
#include "MapTiles.hpp"
#include <fstream>
#include <unordered_map>
#include <functional>
#include "Util/Logger.hpp"

void LevelManager::LoadLevel(const std::string& filepath, InteractableManager& interactableManager) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        LOG_ERROR("Error opening file: " + filepath);
        return;
    }

    // 定義字元與對應的處理邏輯
    std::unordered_map<char, std::function<void(int, int)>> featureHandlers = {
        {'F', [&](int x, int y) { m_DefenderSpawn = { x, y }; }},
        {'A', [&](int x, int y) { m_AttackerSpawns.push_back({ x, y }); }},
        {'K', [&](int x, int y) { interactableManager.AddKey(x, y); }},
        {'9', [&](int x, int y) { interactableManager.AddChest(x, y); }},
        {'U', [&](int x, int y) { interactableManager.AddConveyor(x, y, Player::Direction::UP); }},
        {'D', [&](int x, int y) { interactableManager.AddConveyor(x, y, Player::Direction::DOWN); }},
        {'L', [&](int x, int y) { interactableManager.AddConveyor(x, y, Player::Direction::LEFT); }},
        {'R', [&](int x, int y) { interactableManager.AddConveyor(x, y, Player::Direction::RIGHT); }},
        {'4', [&](int x, int y) { interactableManager.AddBouncePad(x, y, Player::Direction::UP); }},
        {'5', [&](int x, int y) { interactableManager.AddBouncePad(x, y, Player::Direction::DOWN); }},
        {'6', [&](int x, int y) { interactableManager.AddBouncePad(x, y, Player::Direction::LEFT); }},
        {'7', [&](int x, int y) { interactableManager.AddBouncePad(x, y, Player::Direction::RIGHT); }},
        {'S', [&](int x, int y) { m_SpiritSpawns.push_back({ x, y }); }}
    };

    m_TileGrid.assign(17, std::vector<std::shared_ptr<Tile>>(25));

    m_Tiles.clear();
    m_AttackerSpawns.clear();

    for (int y=0;y<17;y++) {
        for (int x=0;x<25;x++) {
            char type; 
            file >> type;

            std::shared_ptr<Tile> tile;  // 這個地圖方塊

            if (type == '1') {  // 鋪設無敵牆
                tile = std::make_shared<Wall>(x, y);
            }
            else if (type == '2') {   // 鋪設磚塊
                tile = std::make_shared<Brick>(x, y);
            }
            else {
                // 預設為草地
                tile = std::make_shared<Ground>(x, y);
                
                // 檢查是否有對應的特殊物件或設定
                auto it = featureHandlers.find(type);
                if (it != featureHandlers.end()) {
                    it->second(x, y);
                }
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

    return m_TileGrid[gridY][gridX]->IsDestructible();
}

// 摧毀磚塊並長出草地
void LevelManager::DestroyBrick(int gridX, int gridY, Util::Renderer& root, InteractableManager& interactableManager) {
    if (!IsBrick(gridX, gridY)) return;

    auto oldBrick = m_TileGrid[gridY][gridX];  // 待摧毀的方塊

    root.RemoveChild(oldBrick);

    for (auto it = m_Tiles.begin(); it != m_Tiles.end(); ++it) {
        if (*it == oldBrick) {
            m_Tiles.erase(it);
            break;
        }
    }

    auto newGround = std::make_shared<Ground>(gridX, gridY);
    m_TileGrid[gridY][gridX] = newGround; 
    m_Tiles.push_back(newGround);
    root.AddChild(newGround);

	interactableManager.OnBrickDestroyed(gridX, gridY, root);  // 讓互動物件管理器知道磚塊被摧毀了
}

void LevelManager::Clear(Util::Renderer& root) {
    DetachFromRoot(root);

    m_Tiles.clear();
    m_AttackerSpawns.clear();
    m_SpiritSpawns.clear();
    m_TileGrid.assign(17, std::vector<std::shared_ptr<Tile>>(25));
}