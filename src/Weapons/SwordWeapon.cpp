#include "Weapons/SwordWeapon.hpp"

#include "GameTypes.hpp"
#include "GridCoord.hpp"
#include "Player.hpp"
#include "Weapons/IWeaponEffects.hpp"

int SwordWeapon::Fire(Player& defender, std::vector<std::shared_ptr<Player>>& players,
                      LevelManager& /*level*/, Util::Renderer& /*root*/, IWeaponEffects& fx) {
    const GridOffset o = kCardinalOffsets[static_cast<int>(defender.GetDirection())];
    const int fxg = defender.GetGridX() + o.dx;
    const int fyg = defender.GetGridY() + o.dy;
    const int px = -o.dy, py = o.dx;  // 垂直於面向
    const int tiles[3][2] = { {fxg, fyg}, {fxg + px, fyg + py}, {fxg - px, fyg - py} };

    int kills = 0;
    for (auto& t : tiles) {
        for (auto& p : players) {
            if (p->GetTeam() == Team::ATTACKER && !p->IsDead() &&
                p->GetGridX() == t[0] && p->GetGridY() == t[1]) {
                p->Kill();
                if (p->IsDead()) ++kills;
            }
        }
        const glm::vec2 pix = GridCoord::ToPixel(t[0], t[1]);
        fx.AddEffect(pix.x, pix.y, RESOURCE_DIR"/Image/fx_slash.png", 18);
    }
    return kills;
}
