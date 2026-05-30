#ifndef UIIMAGE_HPP
#define UIIMAGE_HPP

#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include <string>
#include <memory>

class UIImage : public Util::GameObject {
public:
    UIImage(const std::string& imagePath, float x = 0.0f, float y = 0.0f, float z = 99.0f);

    // 不載入圖片的建構子：之後以 SetDrawable() 指派 (共用預載的 Util::Image)。
    UIImage(float x, float y, float z = 99.0f);

    void SetPosition(float x, float y);
    void SetScale(float sx, float sy);
    void SetRotation(float radians);
    
    // 讓圖片鋪滿全螢幕
    void SetFullScreen(float targetWidth = 1280.0f, float targetHeight = 720.0f);
};

#endif
