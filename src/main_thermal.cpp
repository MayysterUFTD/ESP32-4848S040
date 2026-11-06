#include <Arduino.h>
#include <Wire.h>
#include <math.h>

void setup()
{
    Serial0.begin(115200);
    delay(100);
    Serial0.println("MLX90640 ready; broadcasting 32x24 WDCP thermal frames on channel 1");
}

void loop()
{
    // wdcp_espnow_process(&espnowTransport);
    // sendThermalFrame();
}
