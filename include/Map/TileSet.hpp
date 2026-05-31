#ifndef TILESET_HPP
#define TILESET_HPP

#include <string>
#include <utility>

// 一個關卡主題用的地形貼圖組 (地板 / 牆 / 破壞箱)。仿原版：牆 = 藍色城牆 (無敵)、
// 破壞箱 = 棕色木箱；地板隨主題變。新增主題只需在 ForLevel 加一個 case (OCP)。
class TileSet {
public:
    TileSet(std::string ground, std::string wall, std::string brick)
        : m_Ground(std::move(ground)), m_Wall(std::move(wall)), m_Brick(std::move(brick)) {}

    const std::string& Ground() const { return m_Ground; }
    const std::string& Wall()   const { return m_Wall; }   // 藍色城牆 (無敵)
    const std::string& Brick()  const { return m_Brick; }  // 棕色木箱 (可破壞)

    // 1 炸彈節(綠草/棕磚)、2 植物基地(森林綠/藍磚)、3 磐石論壇(沙土/石磚)。
    // 仿原版：每關的地板、無敵牆 (wall*)、可破壞磚 (brick*) 主題各異 (OCP：加關只需多一個 case)。
    static TileSet ForLevel(int level) {
        switch (level) {
            case 2:  return TileSet(RESOURCE_DIR"/Image/ground_l2.png", RESOURCE_DIR"/Image/wall_l2.png", RESOURCE_DIR"/Image/brick_l2.png");
            case 3:  return TileSet(RESOURCE_DIR"/Image/ground_l3.png", RESOURCE_DIR"/Image/wall_l3.png", RESOURCE_DIR"/Image/brick_l3.png");
            default: return TileSet(RESOURCE_DIR"/Image/ground.png",    RESOURCE_DIR"/Image/wall.png",    RESOURCE_DIR"/Image/brick.png");
        }
    }

private:
    std::string m_Ground, m_Wall, m_Brick;
};

#endif
