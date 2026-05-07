#ifndef BACKGROUNDIMAGE_HPP
#define BACKGROUNDIMAGE_HPP

#include "Util/GameObject.hpp"
#include "Util/Image.hpp"

class BackgroundImage : public Util::GameObject {
public:
    BackgroundImage(const std::string& imagePath) {
        auto image = std::make_shared<Util::Image>(imagePath);
        SetDrawable(image);
        SetZIndex(99);

        float targetWidth = 1280.0f;
        float targetHeight = 720.0f;

        m_Transform.scale = {
            targetWidth / image->GetSize().x,
            targetHeight / image->GetSize().y
        };

        m_Transform.translation = { 0.0f, 0.0f };
    }
};

#endif