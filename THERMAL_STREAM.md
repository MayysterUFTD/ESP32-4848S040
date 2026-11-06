# MLX90640 thermal stream

## Wiring

Connect the MLX90640 to the ESP32-S3 using 3.3 V, SDA on GPIO 8, and SCL on
GPIO 9. The firmware runs I2C at 400 kHz and configures the sensor for a 32 Hz
refresh rate. Use short wires and 2.2-4.7 kΩ pull-ups to 3.3 V on SDA and SCL
if frame reads fail with I2C timeouts.

## Wireless format

The sender broadcasts WDCP v3 fragmented frames over ESP-NOW on channel 1:

- WDCP source node: `0x0301`; destination: `0xFFFF`.
- Reassembled payload length: 774 bytes.
- Bytes 0-5: ASCII `TH`, format version `1`, width `32`, height `24`, encoding `2`.
- Remaining bytes: 768 row-major unsigned temperatures, one byte per pixel,
  with a 1°C step and an offset of -40°C. Values 0-254 represent -40°C to
  214°C; `255` denotes an invalid pixel. Measurements outside the range are
  clamped.

WDCP fragmentation splits each image into four ESP-NOW packets. The receiver
validates and reassembles them before displaying the image as a translucent
overlay on its existing dashboard. Both devices must use the same Wi-Fi
channel.

## Build

Build the sender from this project:

```sh
pio run -e esp32-s3-devkitc-1-n8r8
```

Build the receiver from `UFTD-Tire-Temperature-Screen`:

```sh
pio run -e LVGL-320-480
```
