#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <Arduino_GFX_Library.h>
#include <lvgl.h>
#include <cstdint>
#include <cstddef>

namespace display
{
    constexpr int SCREEN_WIDTH = 480;
    constexpr int SCREEN_HEIGHT = 480;
    constexpr int BACKLIGHT_PIN = 38;
    constexpr uint8_t TOUCH_ADDRESS = 0x5D;
    constexpr uint16_t TOUCH_STATUS_REGISTER = 0x814E;
    constexpr uint16_t TOUCH_POINT_REGISTER = 0x814F;

    extern Arduino_RGB_Display *display;
    extern uint16_t drawBuffer[];

    void initialize();
    void flushDisplay(lv_display_t *lvDisplay, const lv_area_t *area, uint8_t *pixels);
    void readTouch(lv_indev_t *, lv_indev_data_t *data);
    uint32_t getTick();
} // namespace display

#endif // DISPLAY_DRIVER_H
