#ifndef BACKGROUNDIMAGE_HPP
#define BACKGROUNDIMAGE_HPP

#include "Util/GameObject.hpp"
#include "Util/Image.hpp"

// 繼承 Util::GameObject
class BackgroundImage : public Util::GameObject {
public:
    BackgroundImage() {
        // 在建構子內直接綁定圖片與圖層高度
        SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR"/Image/cover.jpg"));
        SetZIndex(0);
    }
};

#endif