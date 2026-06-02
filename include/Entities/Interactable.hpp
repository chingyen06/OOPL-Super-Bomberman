#ifndef INTERACTABLE_HPP
#define INTERACTABLE_HPP

#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "GameConstants.hpp"
#include "GameTypes.hpp"  // Direction
#include "Effects/IPlayerEffect.hpp"
#include <functional>
#include <memory>
#include <string>
#include <utility>

class Player;

class Interactable : public Util::GameObject {
public:
    virtual ~Interactable() = default;
    virtual void Update() {};

    virtual int GetGridX() const = 0;
    virtual int GetGridY() const = 0;

    virtual bool IsBlocksBomb() const { return false; }
    virtual bool IsBlocksFire() const { return false; }
    virtual bool IsDestroyedByFire() const { return false; }

    virtual bool OnInteract(Player& player) = 0;

    virtual glm::vec2 GetForce() const { return { 0.0f, 0.0f }; }

    // AI 使用：本 interactable 對攻擊方 bot 而言的目標優先順序。
    // 0 = 不應被選為目標；正數越小代表越優先 (1 = 最優先)。
    // 預設不可選；具體子類覆寫提供自己的邏輯 (e.g. Key 在 bot 已持鑰匙時回 0)。
    // 透過此 hook 取代過往 AIManager 內 dynamic_pointer_cast<PowerUp/Chest/Key> 寫死的型別分支。
    virtual int GetAttackerTargetPriority(bool /*botHasKey*/) const { return 0; }

    // 勝負/HUD 用：本 interactable 是否為「攻擊方需要完成的計分目標」，以及是否已完成。
    // 取代 InteractableManager 內 dynamic_pointer_cast<Chest> 的型別判斷 —
    // 新增一種目標型別 (e.g. 要炸毀的基地) 只需 override 這兩個 hook，
    // 不必再動 InteractableManager 的目標計數 / 狀態查詢 / 勝負邏輯 (OCP)。
    virtual bool IsScoringObjective() const { return false; }
    virtual bool IsObjectiveComplete() const { return false; }

    // Debug 用：強制把本目標標記為完成 (e.g. 主控台「強制進攻方獲勝」)。預設空操作；
    // 具體目標型別覆寫之 (Chest → Open)。同樣以虛擬 hook 取代型別判斷，符合 OCP。
    virtual void ForceComplete() {}
};

class Key : public Interactable {
public:
    Key(int gridX, int gridY);

    int GetGridX() const override { return m_GridX; }
    int GetGridY() const override { return m_GridY; }

    bool IsBlocksBomb() const override { return true; }

    bool OnInteract(Player& player) override;

    // Key 優先序：bot 已持鑰匙 → 不再撿；否則為次要目標 (僅次於 PowerUp)
    int GetAttackerTargetPriority(bool botHasKey) const override {
        return botHasKey ? 0 : 2;
    }

private:
    int m_GridX;
    int m_GridY;
};

class Chest : public Interactable {
public:
    Chest(int gridX, int gridY);

    int GetGridX() const override { return m_GridX; }
    int GetGridY() const override { return m_GridY; }
    bool IsOpened() const { return m_Opened; }

    bool IsBlocksBomb() const override { return true; }
    bool IsBlocksFire() const override { return true; }

    // Chest 是攻擊方的計分目標；開啟即視為完成
    bool IsScoringObjective() const override { return true; }
    bool IsObjectiveComplete() const override { return m_Opened; }
    void ForceComplete() override { Open(); }  // debug 強制開啟

    bool OnInteract(Player& player) override;

    // Chest 優先序：必須有鑰匙、且未開過，才是 bot 的目標 (priority 2 — 與 Key 同層)
    int GetAttackerTargetPriority(bool botHasKey) const override {
        return (botHasKey && !m_Opened) ? 2 : 0;
    }

    void Open();

private:
    int m_GridX;
    int m_GridY;
    bool m_Opened = false;

    std::shared_ptr<Util::Image> m_ClosedImage;
    std::shared_ptr<Util::Image> m_OpenedImage;
};

// Power-up：撿到時對 player 套用一個 IPlayerEffect。本身不再 abstract，
// 新增一種道具只需在 LootTable 註冊新 effect + sprite，不必再為每種道具寫 subclass。
class PowerUp : public Interactable {
public:
    PowerUp(int gridX, int gridY, std::unique_ptr<IPlayerEffect> effect, const std::string& imagePath);

    int GetGridX() const override { return m_GridX; }
    int GetGridY() const override { return m_GridY; }

    bool IsBlocksBomb() const override { return true; }
    bool IsDestroyedByFire() const override { return true; }

    bool OnInteract(Player& player) override;  // 模板方法：呼叫 m_Effect->Apply

    // PowerUp 一律為 bot 最優先目標 (priority 1)
    int GetAttackerTargetPriority(bool /*botHasKey*/) const override { return 1; }

protected:
    int m_GridX;
    int m_GridY;
    std::unique_ptr<IPlayerEffect> m_Effect;
};

// Factory Pattern for Interactables
class InteractableFactory {
public:
    virtual ~InteractableFactory() = default;
    virtual std::shared_ptr<Interactable> Create(int gridX, int gridY) = 0;
};

// 用於 PowerUp 的泛型 Factory：把「如何造 effect」與「sprite 路徑」用建構子注入，
// 不必每加一種 powerup 就新增一個 Factory 子類 (OCP 友善)。
class GenericPowerUpFactory : public InteractableFactory {
public:
    GenericPowerUpFactory(std::function<std::unique_ptr<IPlayerEffect>()> effectMaker, std::string spritePath)
        : m_EffectMaker(std::move(effectMaker)), m_Sprite(std::move(spritePath)) {}

    std::shared_ptr<Interactable> Create(int gridX, int gridY) override {
        return std::make_shared<PowerUp>(gridX, gridY, m_EffectMaker(), m_Sprite);
    }

private:
    std::function<std::unique_ptr<IPlayerEffect>()> m_EffectMaker;
    std::string m_Sprite;
};

// Empty drop (磚塊摧毀後不掉東西)
class EmptyDropFactory : public InteractableFactory {
public:
    std::shared_ptr<Interactable> Create(int /*gridX*/, int /*gridY*/) override {
        return nullptr; // Drop nothing
    }
};

// Conveyor
class Conveyor : public Interactable {
public:
    Conveyor(int gridX, int gridY, Direction dir);
    int GetGridX() const override { return m_GridX; }
    int GetGridY() const override { return m_GridY; }

    bool OnInteract(Player& /*player*/) override { return false; }

    glm::vec2 GetForce() const override;

private:
    int m_GridX;
    int m_GridY;
    Direction m_Dir;
};

// BouncePad
class BouncePad : public Interactable {
public:
    BouncePad(int gridX, int gridY, Direction dir);
    void Update() override;

    int GetGridX() const override { return m_GridX; }
    int GetGridY() const override { return m_GridY; }

    bool OnInteract(Player& player) override;

private:
    int m_GridX;
    int m_GridY;
    Direction m_Dir;
    int m_Distance = Constants::BouncePad::kDefaultDistance;
    int m_Cooldown = 0;

    std::shared_ptr<Util::Image> m_ActiveImage;
    std::shared_ptr<Util::Image> m_InactiveImage;

};

#endif
