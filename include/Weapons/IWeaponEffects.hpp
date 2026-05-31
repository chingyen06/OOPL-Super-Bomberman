#ifndef IWEAPONEFFECTS_HPP
#define IWEAPONEFFECTS_HPP

#include <string>

// 武器特效的「接收端」抽象：武器只負責決定要在哪生成什麼特效，
// 由實作者 (GameSession) 負責生命週期 (DIP — 武器不依賴具體場景管理)。
class IWeaponEffects {
public:
    virtual ~IWeaponEffects() = default;
    // 在像素座標生成一個壽命 frames 的特效圖
    virtual void AddEffect(float px, float py, const std::string& sprite, int frames) = 0;
};

#endif
