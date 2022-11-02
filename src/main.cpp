#include <Arduino.h>
#include "DeviceNextGen/DeviceNextGen.h"

static Device device;

void setup() {
  // put your setup code here, to run once:
  device.init();
  device.start();
}

void loop() {
  // put your main code here, to run repeatedly:
  vTaskDelay(pdMS_TO_TICKS(2000));
  yield();
}