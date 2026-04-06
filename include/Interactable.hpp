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

    virtual bool OnInteract(std::shared_ptr<Player>& player) = 0;
};

class Key : public Interactable {
public:
    Key(int gridX, int gridY);

    int GetGridX() const { return m_GridX; }
    int GetGridY() const { return m_GridY; }

    bool IsBlocksFire() const override { return false; }

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

    bool OnInteract(std::shared_ptr<Player>& player) override;

    void Open();

private:
    int m_GridX;
    int m_GridY;
    bool m_Opened = false;

    std::shared_ptr<Util::Image> m_ClosedImage;
    std::shared_ptr<Util::Image> m_OpenedImage;
};

#endif