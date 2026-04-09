#ifndef INTERACTABLE_HPP
#define INTERACTABLE_HPP

#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include "Player.hpp"
#include <memory>

class player;

class Interactable : public Util::GameObject {
public:
    virtual ~Interactable() = default;
    virtual int GetGridX() const = 0;
    virtual int GetGridY() const = 0;

    virtual bool IsBlocksFire() const = 0;
    virtual bool IsDestroyedByFire() const = 0;

    virtual bool OnInteract(std::shared_ptr<Player>& player) = 0;
};

class Key : public Interactable {
public:
    Key(int gridX, int gridY);

    int GetGridX() const { return m_GridX; }
    int GetGridY() const { return m_GridY; }

    bool IsBlocksFire() const override { return false; }
    bool IsDestroyedByFire() const override { return false; }

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

    bool IsBlocksFire() const override { return true; }
    bool IsDestroyedByFire() const override { return false; }

    bool OnInteract(std::shared_ptr<Player>& player) override;

    void Open();

private:
    int m_GridX;
    int m_GridY;
    bool m_Opened = false;

    std::shared_ptr<Util::Image> m_ClosedImage;
    std::shared_ptr<Util::Image> m_OpenedImage;
};

// 道具
class PowerUp : public Interactable {
public:
    PowerUp(int gridX, int gridY);
    int GetGridX() const override { return m_GridX; }
    int GetGridY() const override { return m_GridY; }

    // 道具不會擋火，但會被火燒毀
    bool IsBlocksFire() const override { return false; }
    bool IsDestroyedByFire() const override { return true; }

protected:
    int m_GridX;
    int m_GridY;
};

// 加速鞋道具
class SpeedItem : public PowerUp {
public:
    SpeedItem(int gridX, int gridY);
    bool OnInteract(std::shared_ptr<Player>& player) override;
};

// Factory Pattern for Interactables
class InteractableFactory {
public:
    virtual ~InteractableFactory() = default;
    virtual std::shared_ptr<Interactable> Create(int gridX, int gridY) = 0;
};

// 加速鞋
class SpeedItemFactory : public InteractableFactory {
public:
    std::shared_ptr<Interactable> Create(int gridX, int gridY) override {
        return std::make_shared<SpeedItem>(gridX, gridY);
    }
};

// 空氣
class EmptyDropFactory : public InteractableFactory {
public:
    std::shared_ptr<Interactable> Create(int gridX, int gridY) override {
        return nullptr; // 什麼都不掉
    }
};

#endif