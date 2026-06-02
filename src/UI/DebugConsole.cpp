#include "UI/DebugConsole.hpp"

#include <imgui.h>

#include "CheatManager.hpp"
#include "Core/DefenderWeaponSystem.hpp"
#include "GameConstants.hpp"
#include "InteractableManager.hpp"
#include "Managers/BombManager.hpp"
#include "Player.hpp"
#include "SaveData.hpp"
#include "Spirit.hpp"
#include "Turret/TurretManager.hpp"
#include "Util/Renderer.hpp"

void DebugConsole::Render(int level, int& gameTimeTicks, float fps,
                          InteractableManager& interactables, SaveData* profile,
                          std::vector<std::shared_ptr<Player>>& players,
                          std::vector<std::shared_ptr<Spirit>>& spirits,
                          Player* humanPlayer1, Player* humanPlayer2,
                          CheatManager& cheat, BombManager& bombs, TurretManager& turrets,
                          DefenderWeaponSystem& weapon, Util::Renderer& root) {
    ImGui::SetNextWindowPos(ImVec2(12, 12), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(320, 0), ImGuiCond_FirstUseEver);
    ImGui::Begin("Game Debug Console (F3)");

    if (ImGui::CollapsingHeader("FPS Panel", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("FPS: %.1f", fps);
    }

    if (ImGui::CollapsingHeader("Match", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Level: %d", level);
        ImGui::Text("Time left: %d s", (gameTimeTicks > 0 ? gameTimeTicks : 0) / Constants::Game::kFPS);
        int opened = 0;
        for (bool done : interactables.GetObjectiveStatusList()) if (done) opened++;
        ImGui::Text("Chests opened: %d / %d", opened, interactables.GetObjectiveCount());
        if (ImGui::Button("+30s")) gameTimeTicks += 30 * Constants::Game::kFPS;
        ImGui::SameLine();
        if (ImGui::Button("-30s")) { gameTimeTicks -= 30 * Constants::Game::kFPS; if (gameTimeTicks < 0) gameTimeTicks = 0; }
        ImGui::SameLine();
        if (ImGui::Button("End now (Defender Win)")) gameTimeTicks = 0;

        if (ImGui::Button("Force Attacker Win (open all chests)")) interactables.ForceCompleteObjectives();
        ImGui::Checkbox("Freeze timer", &m_FreezeTimer);
        ImGui::SameLine();
        if (ImGui::Button("Fill weapon charge")) weapon.DebugFillCharge();
    }

    if (profile && ImGui::CollapsingHeader("Player Wallet", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Coins: %d", profile->Coins());
        if (ImGui::Button("+100"))  profile->AddCoins(100);  ImGui::SameLine();
        if (ImGui::Button("+500"))  profile->AddCoins(500);  ImGui::SameLine();
        if (ImGui::Button("+1000")) profile->AddCoins(1000);
        if (ImGui::Button("Reset (0)")) profile->SetCoins(0);
        ImGui::SameLine();
        static int amount = 50;
        ImGui::SetNextItemWidth(90);
        ImGui::InputInt("##setcoins", &amount);
        ImGui::SameLine();
        if (ImGui::Button("Set")) profile->SetCoins(amount);
    }

    if (ImGui::CollapsingHeader("Players", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Players: %d   Spirits: %d",
                    static_cast<int>(players.size()), static_cast<int>(spirits.size()));
        auto playerLine = [](const char* tag, Player* p) {
            ImGui::Text("%s: grid(%d,%d)  lives %d  fp %d  %s", tag,
                        p->GetGridX(), p->GetGridY(), p->GetLives(), p->GetFirepower(),
                        p->IsDead() ? "DEAD" : (p->IsStunned() ? "STUN" : "OK"));
        };
        // 每位玩家的即時控制鈕 (PushID 區隔同名按鈕)
        auto playerControls = [](Player* p) {
            ImGui::PushID(p);
            if (ImGui::SmallButton("Speed"))   p->ActivateSpeedBoost(); ImGui::SameLine();
            if (ImGui::SmallButton("+Bomb"))   p->IncreaseMaxBombs();   ImGui::SameLine();
            if (ImGui::SmallButton("+Fire"))   p->IncreaseFirepower();  ImGui::SameLine();
            if (ImGui::SmallButton("Respawn")) p->Respawn();
            // 鑰匙是進攻方專屬 (開寶箱)，只對進攻方顯示
            if (p->GetTeam() == Team::ATTACKER) {
                if (ImGui::SmallButton("Give Key")) p->SetKey(true);  ImGui::SameLine();
                if (ImGui::SmallButton("Take Key")) p->SetKey(false);
            }
            ImGui::PopID();
        };
        if (humanPlayer1) { playerLine("P1", humanPlayer1); playerControls(humanPlayer1); }
        if (humanPlayer2) { playerLine("P2", humanPlayer2); playerControls(humanPlayer2); }
        else              ImGui::TextDisabled("P2: (no human P2)");

        bool c1 = cheat.IsEnabled(0);
        if (ImGui::Checkbox("Cheat P1 (god / max)", &c1)) cheat.Toggle(0);
        if (humanPlayer2) {
            bool c2 = cheat.IsEnabled(1);
            if (ImGui::Checkbox("Cheat P2 (god / max)", &c2)) cheat.Toggle(1);
        }
        else {
            ImGui::TextDisabled("Cheat P2 (no human P2)");
        }
        // Kill 無視無敵時間 (DebugKill)
        if (ImGui::Button("Kill P1") && humanPlayer1) humanPlayer1->DebugKill();
        if (humanPlayer2) { ImGui::SameLine(); if (ImGui::Button("Kill P2")) humanPlayer2->DebugKill(); }
        if (ImGui::Button("Kill all attackers")) {
            for (auto& p : players)
                if (p->GetTeam() == Team::ATTACKER && !p->IsDead()) p->DebugKill();
        }
    }

    if (ImGui::CollapsingHeader("Spawn / Clear", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (humanPlayer1) {
            if (ImGui::Button("Spawn bomb @P1")) {
                // ownerID = -1：不歸屬玩家，爆炸不影響 P1 炸彈計數
                bombs.SpawnBomb(humanPlayer1->GetGridX(), humanPlayer1->GetGridY(),
                                humanPlayer1->GetFirepower(), -1, root);
            }
        }
        else {
            ImGui::TextDisabled("(no P1 to anchor spawns)");
        }
        if (ImGui::Button("Clear all bombs")) bombs.Clear(root);
        ImGui::SameLine();
        if (ImGui::Button("Clear turrets"))   turrets.Clear(root);
        ImGui::SameLine();
        if (ImGui::Button("Clear spirits")) {
            for (auto& s : spirits) root.RemoveChild(s);
            spirits.clear();
        }
    }

    if (ImGui::CollapsingHeader("Visualize", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Show danger map (red tiles)", &m_ShowDanger);
    }

    ImGui::End();
}
