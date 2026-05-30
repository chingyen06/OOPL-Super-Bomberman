#ifndef LEVELMANAGER_HPP
#define LEVELMANAGER_HPP

#include <vector>
#include <string>
#include <memory>
#include <utility>
#include "GridCoord.hpp"
#include "Util/GameObject.hpp"
#include "Util/Renderer.hpp"
#include "MapTiles.hpp"
#include "TileSet.hpp"

class InteractableManager;

class LevelManager {
public:
    void LoadLevel(const std::string& filepath, const TileSet& tileset, InteractableManager& interactableManager, Util::Renderer& root);
    void AttachToRoot(Util::Renderer& root);
    void DetachFromRoot(Util::Renderer& root);
    bool IsWalkable(int gridX, int gridY) const;  // Query whether (gridX, gridY) is walkable
	bool IsBrick(int gridX, int gridY) const;  // Query whether (gridX, gridY) is a destructible brick
	void DestroyBrick(int gridX, int gridY, Util::Renderer& root, InteractableManager& interactableManager);  // Destroy the brick at (gridX, gridY)
	void Clear(Util::Renderer& root);

    std::pair<int, int> GetDefenderSpawn() const { return m_DefenderSpawn; }
    std::vector<std::pair<int, int>> GetAttackerSpawns() const { return m_AttackerSpawns; }
    int GetMapWidth() const { return m_TileGrid.empty() ? GridCoord::kMapWidth : static_cast<int>(m_TileGrid[0].size()); }
    int GetMapHeight() const { return m_TileGrid.empty() ? GridCoord::kMapHeight : static_cast<int>(m_TileGrid.size()); }
    std::vector<std::pair<int, int>> GetSpiritSpawns() const { return m_SpiritSpawns; }
    std::vector<std::pair<int, int>> GetTurretSpawns() const { return m_TurretSpawns; }
    
private:
    std::vector<std::shared_ptr<Tile>> m_Tiles;
    std::vector<std::vector<std::shared_ptr<Tile>>> m_TileGrid;  // 2D tile grid (row-major: [y][x])

    std::string m_GroundImage = RESOURCE_DIR"/Image/ground.png";  // 本關草地貼圖 (摧毀磚塊後長出的草地沿用主題)

    std::pair<int, int> m_DefenderSpawn = { 1, 1 };  // Defender spawn point
    std::vector<std::pair<int, int>> m_AttackerSpawns;  // Attacker spawn points
    std::vector<std::pair<int, int>> m_SpiritSpawns;
    std::vector<std::pair<int, int>> m_TurretSpawns;
};

#endif
