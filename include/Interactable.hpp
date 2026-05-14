#ifndef INTERACTABLE_HPP
#define INTERACTABLE_HPP

#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "Player.hpp"
#include <memory>

class Interactable : public Util::GameObject {
public:
    virtual ~Interactable() = default;
    virtual void Update() {};

    virtual int GetGridX() const = 0;
    virtual int GetGridY() const = 0;

    virtual bool IsBlocksBomb() const { return false; }
    virtual bool IsBlocksFire() const { return false; }
    virtual bool IsDestroyedByFire() const { return false; }

    virtual bool OnInteract(std::shared_ptr<Player>& player) = 0;

    virtual glm::vec2 GetForce() const { return { 0.0f, 0.0f }; }
};

class Key : public Interactable {
public:
    Key(int gridX, int gridY);

    int GetGridX() const { return m_GridX; }
    int GetGridY() const { return m_GridY; }

    bool IsBlocksBomb() const override { return true; }

    bool OnInteract(std::shared_ptr<Player>& player) override;

private:
    int m_GridX;
    int m_GridY;
};

class Chest : public Interactable {
public:
    Chest(int gridX, int gridY);

    int GetGridX() const { return m_GridX; }
    int GetGridY() const { return m_GridY; }
    bool IsOpened() const { return m_Opened; }

    bool IsBlocksBomb() const override { return true; }
    bool IsBlocksFire() const override { return true; }

    bool OnInteract(std::shared_ptr<Player>& player) override;

    void Open();

private:
    int m_GridX;
    int m_GridY;
    bool m_Opened = false;

    std::shared_ptr<Util::Image> m_ClosedImage;
    std::shared_ptr<Util::Image> m_OpenedImage;
};

// �D��
class PowerUp : public Interactable {
public:
    PowerUp(int gridX, int gridY);
    int GetGridX() const override { return m_GridX; }
    int GetGridY() const override { return m_GridY; }

    bool IsBlocksBomb() const override { return true; }
    bool IsDestroyedByFire() const override { return true; }

protected:
    int m_GridX;
    int m_GridY;
};

// �[�t�c�D��
class SpeedItem : public PowerUp {
public:
    SpeedItem(int gridX, int gridY);
    bool OnInteract(std::shared_ptr<Player>& player) override;
};

// ���u�D�� 
class BombItem : public PowerUp {
public:
    BombItem(int gridX, int gridY);
    bool OnInteract(std::shared_ptr<Player>& player) override;
};

// ���K�D��
class FireItem : public PowerUp {
public:
    FireItem(int gridX, int gridY);
    bool OnInteract(std::shared_ptr<Player>& player) override;
};

// Factory Pattern for Interactables
class InteractableFactory {
public:
    virtual ~InteractableFactory() = default;
    virtual std::shared_ptr<Interactable> Create(int gridX, int gridY) = 0;
};

// �[�t�c
class SpeedItemFactory : public InteractableFactory {
public:
    std::shared_ptr<Interactable> Create(int gridX, int gridY) override {
        return std::make_shared<SpeedItem>(gridX, gridY);
    }
};

// ���u�D��
class BombItemFactory : public InteractableFactory {
public:
    std::shared_ptr<Interactable> Create(int gridX, int gridY) override {
        return std::make_shared<BombItem>(gridX, gridY);
    }
};

//���K�D��
class FireItemFactory : public InteractableFactory {
public:
    std::shared_ptr<Interactable> Create(int gridX, int gridY) override {
        return std::make_shared<FireItem>(gridX, gridY);
    }
};

// �Ů�
class EmptyDropFactory : public InteractableFactory {
public:
    std::shared_ptr<Interactable> Create(int gridX, int gridY) override {
        return nullptr; // ���򳣤���
    }
};

// ��e�a
class Conveyor : public Interactable {
public:
    Conveyor(int gridX, int gridY, Player::Direction dir);
    int GetGridX() const override { return m_GridX; }
    int GetGridY() const override { return m_GridY; }

    bool OnInteract(std::shared_ptr<Player>& player) override { return false; }

    glm::vec2 GetForce() const override;

private:
    int m_GridX;
    int m_GridY;
    Player::Direction m_Dir;
};

// �u���O
class BouncePad : public Interactable {
public:
    BouncePad(int gridX, int gridY, Player::Direction dir);
    void Update() override;

    int GetGridX() const override { return m_GridX; }
    int GetGridY() const override { return m_GridY; }

    bool OnInteract(std::shared_ptr<Player>& player) override;

private:
    int m_GridX;
    int m_GridY;
    Player::Direction m_Dir;
    int m_Distance = 3;
    int m_Cooldown = 0;

    std::shared_ptr<Util::Image> m_ActiveImage;
    std::shared_ptr<Util::Image> m_InactiveImage;

};

#endif