#ifndef LEVELMANAGER_HPP
#define LEVELMANAGER_HPP

#include <vector>
#include <string>
#include <memory>
#include "Util/GameObject.hpp"
#include "Util/Renderer.hpp"

class LevelManager {
private:
    std::vector<std::shared_ptr<Util::GameObject>> m_Tiles;

public:
    void LoadLevel(const std::string& filepath);
    void AttachToRoot(Util::Renderer& root);
};

#endif