#ifndef LEVELMANAGER_HPP
#define LEVELMANAGER_HPP

#include <vector>
#include <string>
#include <memory>
#include <utility>
#include "Util/GameObject.hpp"
#include "Util/Renderer.hpp"
#include "MapTiles.hpp"

class InteractableManager;

class LevelManager {
public:
    void LoadLevel(const std::string& filepath, InteractableManager& interactableManager);
    void AttachToRoot(Util::Renderer& root);
    void DetachFromRoot(Util::Renderer& root);
    bool IsWalkable(int gridX, int gridY) const;  // �d�߬O�_�i���ʦ� (gridX, gridY)
	bool IsBrick(int gridX, int gridY) const;  // �d�� (gridX, gridY) �O�_���j��
	void DestroyBrick(int gridX, int gridY, Util::Renderer& root, InteractableManager& interactableManager);  // �R�� (gridX, gridY) ���j��
	void Clear(Util::Renderer& root);

    std::pair<int, int> GetDefenderSpawn() const { return m_DefenderSpawn; }
    std::vector<std::pair<int, int>> GetAttackerSpawns() const { return m_AttackerSpawns; }
    int GetMapWidth() const { return m_TileGrid.empty() ? 25 : m_TileGrid[0].size(); }
    int GetMapHeight() const { return m_TileGrid.size(); }
    std::vector<std::pair<int, int>> GetSpiritSpawns() const { return m_SpiritSpawns; }
    std::vector<std::pair<int, int>> GetTurretSpawns() const { return m_TurretSpawns; }
    
private:
    std::vector<std::shared_ptr<Tile>> m_Tiles;
    //std::vector<std::vector<char>> m_MapData;  // �s�a�Ϥ��
    std::vector<std::vector<std::shared_ptr<Tile>>> m_TileGrid;  // �s�a�Ϥ��

    std::pair<int, int> m_DefenderSpawn = { 1, 1 };  // ���u�I
    std::vector<std::pair<int, int>> m_AttackerSpawns;  // �i���I
    std::vector<std::pair<int, int>> m_SpiritSpawns;
    std::vector<std::pair<int, int>> m_TurretSpawns;
};

#endif