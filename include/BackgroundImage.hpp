#ifndef BACKGROUNDIMAGE_HPP
#define BACKGROUNDIMAGE_HPP

#include "Util/GameObject.hpp"
#include "Util/Image.hpp"

// 繼承 Util::GameObject
class BackgroundImage : public Util::GameObject {
public:
    BackgroundImage() {
        SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR"/Image/cover.jpg"));
        SetZIndex(0);
    }
};

#endif