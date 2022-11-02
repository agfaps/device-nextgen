#include <Arduino.h>
#include "DeviceNextGen.h"
#include "MessageProcessor/MessageProcessor.h"

static const char* TAG = "Device";

void Device::init()
{
    Serial.begin(921600);
    while (!Serial);
    Serial.println();
    Serial.println();
    ESP_LOGI(TAG, "Device starting...");
}

void Device::start()
{
    // init MessageProcessor
    auto &messageProcessor = MessageProcessor::getSingleInstance();
    messageProcessor.start();
    // init OtaUpdate
    // init SystemMonitoring
}
