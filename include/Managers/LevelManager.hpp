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

    // 防守方「屏障」武器用：在可走的格子上暫時放一道不可穿越的牆 (到期自動還原成草地)。
    void AddTemporaryWall(int gridX, int gridY, int frames, const std::string& sprite, Util::Renderer& root);
    void TickTemporary(Util::Renderer& root);  // 每幀呼叫：倒數並還原到期的暫時牆

    std::pair<int, int> GetDefenderSpawn() const { return m_DefenderSpawn; }
    std::vector<std::pair<int, int>> GetAttackerSpawns() const { return m_AttackerSpawns; }
    int GetMapWidth() const { return m_TileGrid.empty() ? GridCoord::kMapWidth : static_cast<int>(m_TileGrid[0].size()); }
    int GetMapHeight() const { return m_TileGrid.empty() ? GridCoord::kMapHeight : static_cast<int>(m_TileGrid.size()); }
    std::vector<std::pair<int, int>> GetSpiritSpawns() const { return m_SpiritSpawns; }
    std::vector<std::pair<int, int>> GetTurretSpawns() const { return m_TurretSpawns; }
    
private:
    // 暫時牆 (屏障武器)：記住覆蓋前的原 tile，到期後還原。
    class TempWall {
    public:
        int gridX, gridY, frames;
        std::shared_ptr<Tile> wall;   // 暫時放上的牆
        std::shared_ptr<Tile> saved;  // 被覆蓋的原 tile (通常是草地)
    };

    std::vector<std::shared_ptr<Tile>> m_Tiles;
    std::vector<std::vector<std::shared_ptr<Tile>>> m_TileGrid;  // 2D tile grid (row-major: [y][x])
    std::vector<TempWall> m_TempWalls;

    std::string m_GroundImage = RESOURCE_DIR"/Image/ground.png";  // 本關草地貼圖 (摧毀磚塊後長出的草地沿用主題)

    std::pair<int, int> m_DefenderSpawn = { 1, 1 };  // Defender spawn point
    std::vector<std::pair<int, int>> m_AttackerSpawns;  // Attacker spawn points
    std::vector<std::pair<int, int>> m_SpiritSpawns;
    std::vector<std::pair<int, int>> m_TurretSpawns;
};

#endif
