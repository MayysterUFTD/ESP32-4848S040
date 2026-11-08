#include "display_driver.h"
#include <Wire.h>
#include <Arduino.h>

namespace display
{
    // ST7701 initialization sequence for ESP32-4848S040 panel
    // Verified working configuration with proper timing
    const uint8_t ST7701_INIT[] = {
        BEGIN_WRITE,
        WRITE_COMMAND_8, 0xFF,
        WRITE_BYTES, 5, 0x77, 0x01, 0x00, 0x00, 0x10,

        WRITE_C8_D16, 0xC0, 0x3B, 0x00,
        WRITE_C8_D16, 0xC1, 0x0D, 0x02,
        WRITE_C8_D16, 0xC2, 0x31, 0x05,
        WRITE_C8_D8, 0xCD, 0x00,

        WRITE_COMMAND_8, 0xB0,
        WRITE_BYTES, 16,
        0x00, 0x11, 0x18, 0x0E,
        0x11, 0x06, 0x07, 0x08,
        0x07, 0x22, 0x04, 0x12,
        0x0F, 0xAA, 0x31, 0x18,

        WRITE_COMMAND_8, 0xB1,
        WRITE_BYTES, 16,
        0x00, 0x11, 0x19, 0x0E,
        0x12, 0x07, 0x08, 0x08,
        0x08, 0x22, 0x04, 0x11,
        0x11, 0xA9, 0x32, 0x18,

        WRITE_COMMAND_8, 0xFF,
        WRITE_BYTES, 5, 0x77, 0x01, 0x00, 0x00, 0x11,

        WRITE_C8_D8, 0xB0, 0x60,
        WRITE_C8_D8, 0xB1, 0x32,
        WRITE_C8_D8, 0xB2, 0x07,
        WRITE_C8_D8, 0xB3, 0x80,
        WRITE_C8_D8, 0xB5, 0x49,
        WRITE_C8_D8, 0xB7, 0x85,
        WRITE_C8_D8, 0xB8, 0x21,
        WRITE_C8_D8, 0xC1, 0x78,
        WRITE_C8_D8, 0xC2, 0x78,

        WRITE_COMMAND_8, 0xE0,
        WRITE_BYTES, 3, 0x00, 0x1B, 0x02,

        WRITE_COMMAND_8, 0xE1,
        WRITE_BYTES, 11,
        0x08, 0xA0, 0x00, 0x00,
        0x07, 0xA0, 0x00, 0x00,
        0x00, 0x44, 0x44,

        WRITE_COMMAND_8, 0xE2,
        WRITE_BYTES, 12,
        0x11, 0x11, 0x44, 0x44,
        0xED, 0xA0, 0x00, 0x00,
        0xEC, 0xA0, 0x00, 0x00,

        WRITE_COMMAND_8, 0xE3,
        WRITE_BYTES, 4, 0x00, 0x00, 0x11, 0x11,

        WRITE_C8_D16, 0xE4, 0x44, 0x44,

        WRITE_COMMAND_8, 0xE5,
        WRITE_BYTES, 16,
        0x0A, 0xE9, 0xD8, 0xA0,
        0x0C, 0xEB, 0xD8, 0xA0,
        0x0E, 0xED, 0xD8, 0xA0,
        0x10, 0xEF, 0xD8, 0xA0,

        WRITE_COMMAND_8, 0xE6,
        WRITE_BYTES, 4, 0x00, 0x00, 0x11, 0x11,

        WRITE_C8_D16, 0xE7, 0x44, 0x44,

        WRITE_COMMAND_8, 0xE8,
        WRITE_BYTES, 16,
        0x09, 0xE8, 0xD8, 0xA0,
        0x0B, 0xEA, 0xD8, 0xA0,
        0x0D, 0xEC, 0xD8, 0xA0,
        0x0F, 0xEE, 0xD8, 0xA0,

        WRITE_COMMAND_8, 0xEB,
        WRITE_BYTES, 7,
        0x02, 0x00, 0xE4, 0xE4,
        0x88, 0x00, 0x40,

        WRITE_C8_D16, 0xEC, 0x3C, 0x00,

        WRITE_COMMAND_8, 0xED,
        WRITE_BYTES, 16,
        0xAB, 0x89, 0x76, 0x54,
        0x02, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0x20,
        0x45, 0x67, 0x98, 0xBA,

        WRITE_COMMAND_8, 0xFF,
        WRITE_BYTES, 5, 0x77, 0x01, 0x00, 0x00, 0x13,

        WRITE_C8_D8, 0xE5, 0xE4,

        WRITE_COMMAND_8, 0xFF,
        WRITE_BYTES, 5, 0x77, 0x01, 0x00, 0x00, 0x00,

        WRITE_C8_D8, 0x3A, 0x60,

        DELAY, 10,
        WRITE_COMMAND_8, 0x11,
        END_WRITE,

        DELAY, 120,

        BEGIN_WRITE,
        WRITE_COMMAND_8, 0x29,
        END_WRITE,
    };

    // Display instances
    Arduino_DataBus *commandBus = nullptr;
    Arduino_ESP32RGBPanel *rgbPanel = nullptr;
    Arduino_RGB_Display *display = nullptr;
    
    // Allocate draw buffer in PSRAM to avoid DRAM overflow
    uint16_t *drawBuffer = nullptr;

    // Touch state
    bool touchPressed = false;
    uint32_t lastTouchTime = 0;
    lv_point_t lastTouchPoint = {0, 0};
    uint8_t touchAddress = 0;

    bool detectTouchController()
    {
        constexpr uint8_t addresses[] = {TOUCH_ADDRESS, 0x14};
        for (const uint8_t address : addresses)
        {
            Wire.beginTransmission(address);
            if (Wire.endTransmission() == 0)
            {
                touchAddress = address;
                Serial.printf("GT911 touch controller found at I2C address 0x%02X\n", touchAddress);
                return true;
            }
        }

        touchAddress = 0;
        return false;
    }

    bool readRegister(uint16_t reg, uint8_t *buffer, size_t length)
    {
        if (touchAddress == 0)
        {
            return false;
        }

        Wire.beginTransmission(touchAddress);
        Wire.write(static_cast<uint8_t>(reg >> 8));
        Wire.write(static_cast<uint8_t>(reg & 0xFF));
        if (Wire.endTransmission(false) != 0)
        {
            return false;
        }

        if (Wire.requestFrom(touchAddress, length) != length)
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
        Wire.beginTransmission(touchAddress);
        Wire.write(static_cast<uint8_t>(TOUCH_STATUS_REGISTER >> 8));
        Wire.write(static_cast<uint8_t>(TOUCH_STATUS_REGISTER & 0xFF));
        Wire.write(0);
        Wire.endTransmission();
    }

    void initialize()
    {
        // Initialize command bus (SPI)
        commandBus = new Arduino_SWSPI(
            GFX_NOT_DEFINED, 39, 48, 47, GFX_NOT_DEFINED);

        // Initialize RGB panel with corrected timing parameters
        // Fixing horizontal/vertical sync timing for proper display refresh
        rgbPanel = new Arduino_ESP32RGBPanel(
            18, 17, 16, 21,     // hsync, vsync, de, pclk
            11, 12, 13, 14, 0,   // d0-d4
            8, 20, 3, 46, 9, 10, // d5-d10
            4, 5, 6, 7, 15,      // d11-d15
            1, 10, 8, 50,        // freq (MHz), hfp, hbp, hsw - Horizontal: front porch, back porch, sync width
            1, 10, 8, 20,        // vfp, vbp, vsw - Vertical: front porch, back porch, sync width
            0, 12000000);        // prefer_speed (0=no preference), pclk_active_neg (0=active high)

        // Create display with ST7701 controller
        display = new Arduino_RGB_Display(
            SCREEN_WIDTH, SCREEN_HEIGHT, rgbPanel, 0, true,
            commandBus, GFX_NOT_DEFINED, ST7701_INIT, sizeof(ST7701_INIT));

        if (!display->begin())
        {
            Serial.println("BŁĄD: Nie udało się uruchomić wyświetlacza");
            while (true)
                delay(1000);
        }

        // Allocate draw buffer in PSRAM
        drawBuffer = (uint16_t *)ps_malloc(DRAW_BUFFER_SIZE * sizeof(uint16_t));
        if (!drawBuffer)
        {
            Serial.println("BŁĄD: Nie udało się przydzielić bufora w PSRAM");
            while (true)
                delay(1000);
        }
        Serial.printf("Draw buffer allocated at %p (size: %lu bytes)\n", 
                     drawBuffer, DRAW_BUFFER_SIZE * sizeof(uint16_t));

        // Initialize backlight
        pinMode(BACKLIGHT_PIN, OUTPUT);
        digitalWrite(BACKLIGHT_PIN, HIGH);

        // Initialize touch panel I2C
        Wire.begin(19, 45, 400000);
        if (!detectTouchController())
        {
            Serial.println("ERROR: GT911 not detected at I2C addresses 0x5D or 0x14");
        }

        Serial.println("Display driver initialized successfully");
    }

    void flushDisplay(lv_display_t *lvDisplay, const lv_area_t *area, uint8_t *pixels)
    {
        uint32_t w = lv_area_get_width(area);
        uint32_t h = lv_area_get_height(area);
        
        // Debug: Log EVERY flush to verify it's being called
        static uint32_t flushCount = 0;
        if (++flushCount <= 10 || flushCount % 30 == 0)
        {
            Serial.printf("Flush #%lu: area(%d,%d)-(%d,%d) w=%lu h=%lu pixels=%p\n", 
                         flushCount, area->x1, area->y1, area->x2, area->y2, w, h, pixels);
        }
        
        // Verify pixel data is not null
        if (!pixels)
        {
            Serial.println("ERROR: pixels buffer is NULL!");
            lv_display_flush_ready(lvDisplay);
            return;
        }
        
        // pixels is uint8_t but represents RGB565 data (2 bytes per pixel)
        // Cast to uint16_t* for draw16bitRGBBitmap
        uint16_t *pixelData = reinterpret_cast<uint16_t *>(pixels);
        
        // Draw the bitmap - this is what actually sends data to display
        display->draw16bitRGBBitmap(
            area->x1, area->y1,
            pixelData,
            w, h);
        
        lv_display_flush_ready(lvDisplay);
    }

    void readTouch(lv_indev_t *, lv_indev_data_t *data)
    {
        uint8_t status = 0;
        static uint32_t readCount = 0;
        static uint32_t readFailureCount = 0;
        static uint32_t lastDetectionAttempt = 0;

        if (touchAddress == 0 && millis() - lastDetectionAttempt >= 1000)
        {
            lastDetectionAttempt = millis();
            if (!detectTouchController())
            {
                Serial.println("ERROR: GT911 is still not responding on I2C");
            }
        }
        
        if (readRegister(TOUCH_STATUS_REGISTER, &status, 1))
        {
            // Log every 100 reads to see if we're even detecting touches
            if (++readCount % 100 == 0)
            {
                Serial.printf("Touch read #%lu: status=0x%02X\n", readCount, status);
            }
            
            if (status & 0x80)
            {
                const uint8_t pointCount = status & 0x0F;
                if (pointCount > 0)
                {
                    uint8_t point[8] = {};
                    if (readRegister(TOUCH_POINT_REGISTER, point, sizeof(point)))
                    {
                        const uint16_t x = point[1] | (static_cast<uint16_t>(point[2]) << 8);
                        const uint16_t y = point[3] | (static_cast<uint16_t>(point[4]) << 8);
                        lastTouchPoint.x = x < SCREEN_WIDTH ? x : SCREEN_WIDTH - 1;
                        lastTouchPoint.y = y < SCREEN_HEIGHT ? y : SCREEN_HEIGHT - 1;
                        touchPressed = true;
                        lastTouchTime = millis();
                        
                        Serial.printf("Touch detected: x=%u, y=%u\n", lastTouchPoint.x, lastTouchPoint.y);
                    }
                }
                else
                {
                    touchPressed = false;
                }
                acknowledgeTouch();
            }
        }
        else if (++readFailureCount % 200 == 0)
        {
            Serial.println("ERROR: GT911 status register read failed");
        }
        
        if (millis() - lastTouchTime > 150)
        {
            touchPressed = false;
        }

        data->point = lastTouchPoint;
        data->state = touchPressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    }

    uint32_t getTick()
    {
        return millis();
    }
} // namespace display
