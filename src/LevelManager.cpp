#include "LevelManager.hpp"
#include "GridCoord.hpp"
#include "InteractableManager.hpp"
#include "MapTiles.hpp"
#include <fstream>
#include <unordered_map>
#include <functional>
#include "Util/Logger.hpp"

void LevelManager::LoadLevel(const std::string& filepath, InteractableManager& interactableManager, Util::Renderer& root) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        LOG_ERROR("Error opening file: " + filepath);
        return;
    }

    // LevelManager 只負責「spawn metadata」這類關卡座標 (Defender / Attacker / Spirit / Turret)。
    // 其他「在地圖上會被建立的 interactable 物件」全部交由 InteractableManager::HandleSymbol —
    // LevelManager 不再知道有哪些 interactable 種類存在，新增類型不必動本檔。
    std::unordered_map<char, std::function<void(int, int)>> spawnHandlers = {
        {'F', [&](int x, int y) { m_DefenderSpawn = { x, y }; }},
        {'A', [&](int x, int y) { m_AttackerSpawns.push_back({ x, y }); }},
        {'S', [&](int x, int y) { m_SpiritSpawns.push_back({ x, y }); }},
        {'B', [&](int x, int y) { m_TurretSpawns.push_back({ x, y }); }},
    };

    m_TileGrid.assign(GridCoord::kMapHeight, std::vector<std::shared_ptr<Tile>>(GridCoord::kMapWidth));

    m_Tiles.clear();
    m_AttackerSpawns.clear();

    bool truncated = false;
    for (int y = 0; y < GridCoord::kMapHeight && !truncated; y++) {
        for (int x = 0; x < GridCoord::kMapWidth; x++) {
            char type;
            if (!(file >> type)) {
                LOG_ERROR("Map file truncated at (" + std::to_string(x) + ", " + std::to_string(y) + "): " + filepath);
                truncated = true;
                break;
            }

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

                // 先試 spawn handlers (LevelManager 自己管 spawn 座標)，否則委派給 InteractableManager
                auto it = spawnHandlers.find(type);
                if (it != spawnHandlers.end()) {
                    it->second(x, y);
                } else {
                    interactableManager.HandleSymbol(type, x, y, root);
                }
            }

            m_TileGrid[y][x] = tile;  // 存地圖方塊
            m_Tiles.push_back(tile);  // 鋪設方塊
        }
    }

    // 截斷後剩下的格子一律補草地，避免空 shared_ptr 留在 grid 中
    if (truncated) {
        for (int y = 0; y < GridCoord::kMapHeight; y++) {
            for (int x = 0; x < GridCoord::kMapWidth; x++) {
                if (!m_TileGrid[y][x]) {
                    auto fallback = std::make_shared<Ground>(x, y);
                    m_TileGrid[y][x] = fallback;
                    m_Tiles.push_back(fallback);
                }
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

void LevelManager::DetachFromRoot(Util::Renderer& root) {
    for (auto& tile : m_Tiles) {
        root.RemoveChild(tile);
    }
}

// 碰撞查詢
bool LevelManager::IsWalkable(int gridX, int gridY) const {
    if (!GridCoord::InBounds(gridX, gridY))
        return false;

    // 可走的回傳
    return m_TileGrid[gridY][gridX]->IsPassable();
}

// 檢查該格是否為磚塊
bool LevelManager::IsBrick(int gridX, int gridY) const {
    if (!GridCoord::InBounds(gridX, gridY))
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
    m_TurretSpawns.clear();
    m_TileGrid.assign(GridCoord::kMapHeight, std::vector<std::shared_ptr<Tile>>(GridCoord::kMapWidth));
}