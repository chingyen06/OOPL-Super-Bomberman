#include "UI/UIImage.hpp"
#include "Util/Image.hpp"

UIImage::UIImage(const std::string& imagePath, float x, float y, float z) {
    auto image = std::make_shared<Util::Image>(imagePath);
    SetDrawable(image);
    SetZIndex(z);
    m_Transform.translation = { x, y };
}

void UIImage::SetPosition(float x, float y) {
    m_Transform.translation = { x, y };
}

void UIImage::SetScale(float sx, float sy) {
    m_Transform.scale = { sx, sy };
}

void UIImage::SetFullScreen(float targetWidth, float targetHeight) {
    auto drawable = std::dynamic_pointer_cast<Util::Image>(m_Drawable);
    if (drawable) {
        m_Transform.scale = {
            targetWidth / drawable->GetSize().x,
            targetHeight / drawable->GetSize().y
        };
        m_Transform.translation = { 0.0f, 0.0f };
    }
}
