#ifndef INTERACTABLEMANAGER_HPP
#define INTERACTABLEMANAGER_HPP

#include <vector>
#include <memory>
#include "Interactable.hpp"
#include "Player.hpp"
#include "Util/Renderer.hpp"
#include "Util/Logger.hpp"

class Player;

class InteractableManager {
public:
    void AddKey(int gridX, int gridY);
    void AddChest(int gridX, int gridY);

    void Update(std::shared_ptr<Player>& player, Util::Renderer& root);

    void Clear(Util::Renderer& root);

    bool IsInteractableAt(int gridX, int gridY) const;

    void AttachToRoot(Util::Renderer& root);

private:
    std::vector<std::shared_ptr<Key>> m_Keys;
    std::vector<std::shared_ptr<Chest>> m_Chests;
};

#endif