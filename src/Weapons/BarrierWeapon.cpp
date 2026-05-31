#include "Weapons/BarrierWeapon.hpp"

#include "GameTypes.hpp"
#include "LevelManager.hpp"
#include "Player.hpp"
#include "Weapons/IWeaponEffects.hpp"

int BarrierWeapon::Fire(Player& defender, std::vector<std::shared_ptr<Player>>& /*players*/,
                        LevelManager& level, Util::Renderer& root, IWeaponEffects& /*fx*/) {
    const GridOffset o = kCardinalOffsets[static_cast<int>(defender.GetDirection())];
    const int fxg = defender.GetGridX() + o.dx;
    const int fyg = defender.GetGridY() + o.dy;
    const int px = -o.dy, py = o.dx;  // 垂直於面向 → 生成 3 格寬的牆
    constexpr int kFrames = 60 * 6;   // 屏障維持 6 秒
    const int tiles[3][2] = { {fxg, fyg}, {fxg + px, fyg + py}, {fxg - px, fyg - py} };
    for (auto& t : tiles) {
        level.AddTemporaryWall(t[0], t[1], kFrames, RESOURCE_DIR"/Image/fx_barrier.png", root);
    }
    return 0;  // 屏障不造成擊倒
}
