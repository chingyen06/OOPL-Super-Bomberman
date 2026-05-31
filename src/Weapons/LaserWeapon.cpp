#include "Weapons/LaserWeapon.hpp"

#include "GameTypes.hpp"
#include "GridCoord.hpp"
#include "LevelManager.hpp"
#include "Player.hpp"
#include "Weapons/IWeaponEffects.hpp"

int LaserWeapon::Fire(Player& defender, std::vector<std::shared_ptr<Player>>& players,
                      LevelManager& level, Util::Renderer& /*root*/, IWeaponEffects& fx) {
    const GridOffset o = kCardinalOffsets[static_cast<int>(defender.GetDirection())];
    constexpr int kRange = 6;
    int kills = 0;
    for (int s = 1; s <= kRange; ++s) {
        const int gx = defender.GetGridX() + o.dx * s;
        const int gy = defender.GetGridY() + o.dy * s;
        if (!GridCoord::InBounds(gx, gy)) break;

        const glm::vec2 pix = GridCoord::ToPixel(gx, gy);
        fx.AddEffect(pix.x, pix.y, RESOURCE_DIR"/Image/fx_laser.png", 16);

        for (auto& p : players) {
            if (p->GetTeam() == Team::ATTACKER && !p->IsDead() &&
                p->GetGridX() == gx && p->GetGridY() == gy) {
                p->Kill();
                if (p->IsDead()) ++kills;
            }
        }
        if (!level.IsWalkable(gx, gy)) break;  // 撞牆/磚 → 雷射被擋住
    }
    return kills;
}
