#include "Weapons/BarrierWeapon.hpp"

#include "GameTypes.hpp"
#include "LevelManager.hpp"
#include "Player.hpp"
#include "Weapons/IWeaponEffects.hpp"

int BarrierWeapon::Fire(const FireContext& ctx) {
    Player& defender = ctx.Defender();
    LevelManager& level = ctx.Level();
    const GridOffset o = kCardinalOffsets[static_cast<int>(defender.GetDirection())];
    const int fxg = defender.GetGridX() + o.dx;
    const int fyg = defender.GetGridY() + o.dy;
    const int px = -o.dy, py = o.dx;  // 垂直於面向 → 生成 3 格寬的牆
    constexpr int kFrames = 60 * 6;   // 屏障維持 6 秒

    // 某格正站著玩家時不放牆：牆是不可穿越 tile，蓋在玩家所在格會讓該玩家的四角碰撞
    // 永遠成立而徹底卡死 (往任何方向移動都被自己腳下的牆擋住)。略過該格，其餘照放。
    auto occupied = [&](int gx, int gy) {
        for (const auto& p : ctx.Players())
            if (p && p->GetGridX() == gx && p->GetGridY() == gy) return true;
        return false;
    };

    const int tiles[3][2] = { {fxg, fyg}, {fxg + px, fyg + py}, {fxg - px, fyg - py} };
    for (auto& t : tiles) {
        if (occupied(t[0], t[1])) continue;  // 不要把玩家封進牆裡
        level.AddTemporaryWall(t[0], t[1], kFrames, RESOURCE_DIR"/Image/fx_barrier.png", ctx.Root());
    }
    return 0;  // 屏障不造成擊倒
}
