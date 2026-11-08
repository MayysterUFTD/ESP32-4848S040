#include <Arduino.h>
#include <lvgl.h>
#include <ui/ui.h>
#include "display_driver.h"

namespace
{
    lv_obj_t *counterLabel;
    lv_obj_t *menuScreen;

    // Forward declarations
    void buildTestMenu(lv_obj_t *screen);

    void buttonClicked(lv_event_t *event)
    {
        auto *label = static_cast<lv_obj_t *>(lv_event_get_user_data(event));
        static uint32_t clicks = 0;
        lv_label_set_text_fmt(label, "Kliknięcia: %lu",
                              static_cast<unsigned long>(++clicks));
    }

    void menuItemClicked(lv_event_t *event)
    {
        auto *button = static_cast<lv_obj_t *>(lv_event_get_target(event));
        const char *text = lv_label_get_text(lv_obj_get_child(button, 0));
        Serial.printf("Menu: %s clicked\n", text);
    }

    void buildUi()
    {
        lv_obj_t *screen = lv_screen_active();
        lv_obj_set_style_bg_color(screen, lv_color_hex(0x101820), 0);
        lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

        lv_obj_t *title = lv_label_create(screen);
        lv_label_set_text(title, "ESP32-4848S040");
        lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
        lv_obj_set_style_text_color(title, lv_color_white(), 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

        // Informacje o wyświetlaczu
        lv_obj_t *infoLabel = lv_label_create(screen);
        lv_label_set_text(infoLabel, "Konfiguracja ekranu:\n"
                                     "480x480 @ RGB565\n"
                                     "ST7701 LCD Driver\n"
                                     "CST816 Touch Panel");
        lv_obj_set_style_text_font(infoLabel, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(infoLabel, lv_color_hex(0xB8C7D9), 0);
        lv_obj_align(infoLabel, LV_ALIGN_TOP_LEFT, 20, 120);

        counterLabel = lv_label_create(screen);
        lv_label_set_text(counterLabel, "Kliknięcia: 0");
        lv_obj_set_style_text_color(counterLabel, lv_color_hex(0x00FF00), 0);
        lv_obj_set_style_text_font(counterLabel, &lv_font_montserrat_14, 0);
        lv_obj_align(counterLabel, LV_ALIGN_CENTER, 0, -80);

        // Menu boczne
        lv_obj_t *button = lv_button_create(screen);
        lv_obj_set_size(button, 190, 64);
        lv_obj_align(button, LV_ALIGN_CENTER, 0, 20);
        lv_obj_add_event_cb(button, buttonClicked, LV_EVENT_CLICKED, counterLabel);
        lv_obj_set_style_bg_color(button, lv_color_hex(0x1E90FF), 0);

        lv_obj_t *buttonLabel = lv_label_create(button);
        lv_label_set_text(buttonLabel, "Test");
        lv_obj_center(buttonLabel);

        // Menu Test Entries
        buildTestMenu(screen);
    }

    void buildTestMenu(lv_obj_t *screen)
    {
        lv_obj_t *menuPanel = lv_obj_create(screen);
        lv_obj_set_size(menuPanel, 200, 150);
        lv_obj_align(menuPanel, LV_ALIGN_BOTTOM_LEFT, 20, -20);
        lv_obj_set_style_bg_color(menuPanel, lv_color_hex(0x1E1E1E), 0);
        lv_obj_set_style_border_color(menuPanel, lv_color_hex(0x444444), 0);
        lv_obj_set_style_border_width(menuPanel, 1, 0);

        lv_obj_t *menuTitle = lv_label_create(menuPanel);
        lv_label_set_text(menuTitle, "Menu Testowe");
        lv_obj_set_style_text_font(menuTitle, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(menuTitle, lv_color_hex(0xFFFF00), 0);
        lv_obj_align(menuTitle, LV_ALIGN_TOP_MID, 0, 5);

        const char *menuItems[] = {
            "Test 1: Kolory",
            "Test 2: Dotyk",
            "Test 3: Opóźnienie",
        };

        for (size_t i = 0; i < sizeof(menuItems) / sizeof(menuItems[0]); ++i)
        {
            lv_obj_t *menuButton = lv_button_create(menuPanel);
            lv_obj_set_size(menuButton, 180, 30);
            lv_obj_align(menuButton, LV_ALIGN_TOP_MID, 0, 30 + i * 35);
            lv_obj_set_style_bg_color(menuButton, lv_color_hex(0x2A2A2A), 0);
            lv_obj_add_event_cb(menuButton, menuItemClicked, LV_EVENT_CLICKED, nullptr);

            lv_obj_t *menuItemLabel = lv_label_create(menuButton);
            lv_label_set_text(menuItemLabel, menuItems[i]);
            lv_obj_set_style_text_font(menuItemLabel, &lv_font_montserrat_14, 0);
            lv_obj_center(menuItemLabel);
        }
    }

    void halt(const char *message)
    {
        Serial.println(message);
        while (true)
        {
            delay(1000);
        }
    }

} // namespace

void setup()
{
    Serial.begin(115200);
    delay(100);

    if (!psramFound())
    {
        halt("BŁĄD: Brak PSRAM - ustaw board_build.arduino.memory_type = qio_opi");
    }

    Serial.println("Inicjalizacja wyświetlacza...");
    display::initialize();

    lv_init();
    lv_tick_set_cb(display::getTick);

    lv_display_t *lvDisplay = lv_display_create(
        display::SCREEN_WIDTH, display::SCREEN_HEIGHT);
    lv_display_set_flush_cb(lvDisplay, display::flushDisplay);
    lv_display_set_buffers(
        lvDisplay, display::drawBuffer, nullptr,
        display::DRAW_BUFFER_SIZE * sizeof(uint16_t),
        LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_indev_t *touch = lv_indev_create();
    lv_indev_set_type(touch, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch, display::readTouch);

    ui_init();
    buildUi();

    Serial.println("✓ LVGL 9.6.0 ready");
    Serial.println("✓ Display initialized successfully");
}

void loop()
{
    ui_tick();
    lv_timer_handler();
    delay(5);
}
