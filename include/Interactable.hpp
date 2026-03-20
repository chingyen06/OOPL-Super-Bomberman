#ifndef INTERACTABLE_HPP
#define INTERACTABLE_HPP

#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include <memory>

class Key : public Util::GameObject {
public:
    Key(int gridX, int gridY);

    int GetGridX() const { return m_GridX; }
    int GetGridY() const { return m_GridY; }

private:
    int m_GridX;
    int m_GridY;
};

class Chest : public Util::GameObject {
public:
    Chest(int gridX, int gridY);

    int GetGridX() const { return m_GridX; }
    int GetGridY() const { return m_GridY; }
    bool IsOpened() const { return m_Opened; }

    void Open();

private:
    int m_GridX;
    int m_GridY;
    bool m_Opened = false;

    std::shared_ptr<Util::Image> m_ClosedImage;
    std::shared_ptr<Util::Image> m_OpenedImage;
};

#endif