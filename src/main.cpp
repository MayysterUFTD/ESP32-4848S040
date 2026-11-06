#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include <lvgl.h>
#include <ui/ui.h>

namespace
{

    constexpr int kScreenWidth = 480;
    constexpr int kScreenHeight = 480;
    constexpr int kBacklightPin = 38;
    constexpr uint8_t kTouchAddress = 0x5D;
    constexpr uint16_t kTouchStatusRegister = 0x814E;
    constexpr uint16_t kTouchPointRegister = 0x814F;

    const uint8_t kSt7701Init[] = {
        BEGIN_WRITE,
        WRITE_COMMAND_8,
        0xFF,
        WRITE_BYTES,
        5,
        0x77,
        0x01,
        0x00,
        0x00,
        0x10,
        WRITE_C8_D16,
        0xC0,
        0x3B,
        0x00,
        WRITE_C8_D16,
        0xC1,
        0x0D,
        0x02,
        WRITE_C8_D16,
        0xC2,
        0x31,
        0x05,
        WRITE_C8_D8,
        0xCD,
        0x00,
        WRITE_COMMAND_8,
        0xB0,
        WRITE_BYTES,
        16,
        0x00,
        0x11,
        0x18,
        0x0E,
        0x11,
        0x06,
        0x07,
        0x08,
        0x07,
        0x22,
        0x04,
        0x12,
        0x0F,
        0xAA,
        0x31,
        0x18,
        WRITE_COMMAND_8,
        0xB1,
        WRITE_BYTES,
        16,
        0x00,
        0x11,
        0x19,
        0x0E,
        0x12,
        0x07,
        0x08,
        0x08,
        0x08,
        0x22,
        0x04,
        0x11,
        0x11,
        0xA9,
        0x32,
        0x18,
        WRITE_COMMAND_8,
        0xFF,
        WRITE_BYTES,
        5,
        0x77,
        0x01,
        0x00,
        0x00,
        0x11,
        WRITE_C8_D8,
        0xB0,
        0x60,
        WRITE_C8_D8,
        0xB1,
        0x32,
        WRITE_C8_D8,
        0xB2,
        0x07,
        WRITE_C8_D8,
        0xB3,
        0x80,
        WRITE_C8_D8,
        0xB5,
        0x49,
        WRITE_C8_D8,
        0xB7,
        0x85,
        WRITE_C8_D8,
        0xB8,
        0x21,
        WRITE_C8_D8,
        0xC1,
        0x78,
        WRITE_C8_D8,
        0xC2,
        0x78,
        WRITE_COMMAND_8,
        0xE0,
        WRITE_BYTES,
        3,
        0x00,
        0x1B,
        0x02,
        WRITE_COMMAND_8,
        0xE1,
        WRITE_BYTES,
        11,
        0x08,
        0xA0,
        0x00,
        0x00,
        0x07,
        0xA0,
        0x00,
        0x00,
        0x00,
        0x44,
        0x44,
        WRITE_COMMAND_8,
        0xE2,
        WRITE_BYTES,
        12,
        0x11,
        0x11,
        0x44,
        0x44,
        0xED,
        0xA0,
        0x00,
        0x00,
        0xEC,
        0xA0,
        0x00,
        0x00,
        WRITE_COMMAND_8,
        0xE3,
        WRITE_BYTES,
        4,
        0x00,
        0x00,
        0x11,
        0x11,
        WRITE_C8_D16,
        0xE4,
        0x44,
        0x44,
        WRITE_COMMAND_8,
        0xE5,
        WRITE_BYTES,
        16,
        0x0A,
        0xE9,
        0xD8,
        0xA0,
        0x0C,
        0xEB,
        0xD8,
        0xA0,
        0x0E,
        0xED,
        0xD8,
        0xA0,
        0x10,
        0xEF,
        0xD8,
        0xA0,
        WRITE_COMMAND_8,
        0xE6,
        WRITE_BYTES,
        4,
        0x00,
        0x00,
        0x11,
        0x11,
        WRITE_C8_D16,
        0xE7,
        0x44,
        0x44,
        WRITE_COMMAND_8,
        0xE8,
        WRITE_BYTES,
        16,
        0x09,
        0xE8,
        0xD8,
        0xA0,
        0x0B,
        0xEA,
        0xD8,
        0xA0,
        0x0D,
        0xEC,
        0xD8,
        0xA0,
        0x0F,
        0xEE,
        0xD8,
        0xA0,
        WRITE_COMMAND_8,
        0xEB,
        WRITE_BYTES,
        7,
        0x02,
        0x00,
        0xE4,
        0xE4,
        0x88,
        0x00,
        0x40,
        WRITE_C8_D16,
        0xEC,
        0x3C,
        0x00,
        WRITE_COMMAND_8,
        0xED,
        WRITE_BYTES,
        16,
        0xAB,
        0x89,
        0x76,
        0x54,
        0x02,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0x20,
        0x45,
        0x67,
        0x98,
        0xBA,
        WRITE_COMMAND_8,
        0xFF,
        WRITE_BYTES,
        5,
        0x77,
        0x01,
        0x00,
        0x00,
        0x13,
        WRITE_C8_D8,
        0xE5,
        0xE4,
        WRITE_COMMAND_8,
        0xFF,
        WRITE_BYTES,
        5,
        0x77,
        0x01,
        0x00,
        0x00,
        0x00,
        WRITE_C8_D8,
        0x3A,
        0x60,
        DELAY,
        10,
        WRITE_COMMAND_8,
        0x11,
        END_WRITE,
        DELAY,
        120,
        BEGIN_WRITE,
        WRITE_COMMAND_8,
        0x29,
        END_WRITE,
    };

    Arduino_DataBus *commandBus = new Arduino_SWSPI(
        GFX_NOT_DEFINED, 39, 48, 47, GFX_NOT_DEFINED);

    Arduino_ESP32RGBPanel *rgbPanel = new Arduino_ESP32RGBPanel(
        18, 17, 16, 21,
        11, 12, 13, 14, 0,
        8, 20, 3, 46, 9, 10,
        4, 5, 6, 7, 15,
        1, 10, 8, 50,
        1, 10, 8, 20,
        1, 12000000);

    Arduino_RGB_Display *display = new Arduino_RGB_Display(
        kScreenWidth, kScreenHeight, rgbPanel, 0, true,
        commandBus, GFX_NOT_DEFINED, kSt7701Init, sizeof(kSt7701Init));

    uint16_t drawBuffer[kScreenWidth * 20];
    lv_obj_t *counterLabel;
    bool touchPressed = false;
    uint32_t lastTouchTime = 0;
    lv_point_t lastTouchPoint = {0, 0};

    bool readRegister(uint16_t reg, uint8_t *buffer, size_t length)
    {
        Wire.beginTransmission(kTouchAddress);
        Wire.write(static_cast<uint8_t>(reg >> 8));
        Wire.write(static_cast<uint8_t>(reg & 0xFF));
        if (Wire.endTransmission(false) != 0)
        {
            return false;
        }

        if (Wire.requestFrom(kTouchAddress, length) != length)
        {
            while (Wire.available())
            {
                Wire.read();
            }
            return false;
        }

        for (size_t i = 0; i < length; ++i)
        {
            buffer[i] = static_cast<uint8_t>(Wire.read());
        }
        return true;
    }

    void acknowledgeTouch()
    {
        Wire.beginTransmission(kTouchAddress);
        Wire.write(static_cast<uint8_t>(kTouchStatusRegister >> 8));
        Wire.write(static_cast<uint8_t>(kTouchStatusRegister & 0xFF));
        Wire.write(0);
        Wire.endTransmission();
    }

    void flushDisplay(lv_display_t *lvDisplay, const lv_area_t *area, uint8_t *pixels)
    {
        display->draw16bitRGBBitmap(
            area->x1, area->y1,
            reinterpret_cast<uint16_t *>(pixels),
            lv_area_get_width(area), lv_area_get_height(area));
        lv_display_flush_ready(lvDisplay);
    }

    void readTouch(lv_indev_t *, lv_indev_data_t *data)
    {
        uint8_t status = 0;
        if (readRegister(kTouchStatusRegister, &status, 1) && (status & 0x80))
        {
            const uint8_t pointCount = status & 0x0F;
            if (pointCount > 0)
            {
                uint8_t point[8] = {};
                if (readRegister(kTouchPointRegister, point, sizeof(point)))
                {
                    const uint16_t x = point[1] | (static_cast<uint16_t>(point[2]) << 8);
                    const uint16_t y = point[3] | (static_cast<uint16_t>(point[4]) << 8);
                    lastTouchPoint.x = x < kScreenWidth ? x : kScreenWidth - 1;
                    lastTouchPoint.y = y < kScreenHeight ? y : kScreenHeight - 1;
                    touchPressed = true;
                    lastTouchTime = millis();
                }
            }
            else
            {
                touchPressed = false;
            }
            acknowledgeTouch();
        }
        else if (millis() - lastTouchTime > 150)
        {
            touchPressed = false;
        }

        data->point = lastTouchPoint;
        data->state = touchPressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    }

    uint32_t tick()
    {
        return millis();
    }

    void buttonClicked(lv_event_t *event)
    {
        auto *label = static_cast<lv_obj_t *>(lv_event_get_user_data(event));
        static uint32_t clicks = 0;
        lv_label_set_text_fmt(label, "Kliknięcia: %lu",
                              static_cast<unsigned long>(++clicks));
    }

    void buildUi()
    {
        lv_obj_t *screen = lv_screen_active();
        lv_obj_set_style_bg_color(screen, lv_color_hex(0x101820), 0);
        lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

        lv_obj_t *title = lv_label_create(screen);
        lv_label_set_text(title, "Guition 4848S040");
        lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
        lv_obj_set_style_text_color(title, lv_color_white(), 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 70);

        counterLabel = lv_label_create(screen);
        lv_label_set_text(counterLabel, "Kliknięcia: 0");
        lv_obj_set_style_text_color(counterLabel, lv_color_hex(0xB8C7D9), 0);
        lv_obj_align(counterLabel, LV_ALIGN_CENTER, 0, -20);

        lv_obj_t *button = lv_button_create(screen);
        lv_obj_set_size(button, 190, 64);
        lv_obj_align(button, LV_ALIGN_CENTER, 0, 45);
        lv_obj_add_event_cb(button, buttonClicked, LV_EVENT_CLICKED, counterLabel);

        lv_obj_t *buttonLabel = lv_label_create(button);
        lv_label_set_text(buttonLabel, "Naciśnij");
        lv_obj_center(buttonLabel);
    }

    void halt(const char *message)
    {
        Serial0.println(message);
        while (true)
        {
            delay(1000);
        }
    }

} // namespace

void setup()
{
    Serial0.begin(115200);
    delay(100);

    if (!psramFound())
    {
        halt("Brak PSRAM: ustaw board_build.arduino.memory_type = qio_opi");
    }

    if (!display->begin())
    {
        halt("Nie udalo sie uruchomic wyswietlacza");
    }

    pinMode(kBacklightPin, OUTPUT);
    digitalWrite(kBacklightPin, HIGH);
    Wire.begin(19, 45, 400000);

    lv_init();
    lv_tick_set_cb(tick);

    lv_display_t *lvDisplay = lv_display_create(kScreenWidth, kScreenHeight);
    lv_display_set_flush_cb(lvDisplay, flushDisplay);
    lv_display_set_buffers(
        lvDisplay, drawBuffer, nullptr, sizeof(drawBuffer),
        LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_indev_t *touch = lv_indev_create();
    lv_indev_set_type(touch, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch, readTouch);
    // delay(100);
    ui_init();
    // lv_obj_t *screen = lv_screen_active();
    buildUi();
    Serial0.println("LVGL 9.6.0 ready");
}

void loop()
{
    ui_tick();

    lv_timer_handler();
    delay(5);
}
