#ifndef INTERACTABLE_HPP
#define INTERACTABLE_HPP

#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include <memory>

class Interactable : public Util::GameObject {
public:
    virtual ~Interactable() = default;
    virtual int GetGridX() const = 0;
    virtual int GetGridY() const = 0;

    virtual bool IsBlocksFire() const = 0;
};

class Key : public Interactable {
public:
    Key(int gridX, int gridY);

    int GetGridX() const { return m_GridX; }
    int GetGridY() const { return m_GridY; }

    bool IsBlocksFire() const override { return false; }

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

    void Open();

private:
    int m_GridX;
    int m_GridY;
    bool m_Opened = false;

    std::shared_ptr<Util::Image> m_ClosedImage;
    std::shared_ptr<Util::Image> m_OpenedImage;
};

#endif