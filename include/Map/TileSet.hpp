#ifndef TILESET_HPP
#define TILESET_HPP

#include <string>
#include <utility>

// 一個關卡主題用的地形貼圖組 (地板 / 牆 / 磚)。依關卡選用以做出視覺差異。
// 新增主題只需在 ForLevel 加一個 case (OCP)，不必動 LevelManager / MapTiles。
class TileSet {
public:
    TileSet(std::string ground, std::string wall, std::string brick)
        : m_Ground(std::move(ground)), m_Wall(std::move(wall)), m_Brick(std::move(brick)) {}

    const std::string& Ground() const { return m_Ground; }
    const std::string& Wall()   const { return m_Wall; }
    const std::string& Brick()  const { return m_Brick; }

    // 依關卡索引回傳主題：1 = 草原 (預設)、2 = 沙漠、3 = 冰雪；其餘回預設。
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
