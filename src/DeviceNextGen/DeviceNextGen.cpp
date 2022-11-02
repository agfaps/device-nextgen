#include <Arduino.h>
#include "DeviceNextGen.h"
#include "MessageProcessor/MessageProcessor.h"
#include "OtaUpdate/MqttWifiOtaUpdate.h"
#include "SystemMonitoring/SystemMonitoring.h"

static const char* TAG = "Device";
static MqttWifiOtaUpdate * mqttWifiOtaUpdate;
static SystemMonitoring * systemMonitoring;

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
    mqttWifiOtaUpdate = new MqttWifiOtaUpdate();
    mqttWifiOtaUpdate->start();

    // init SystemMonitoring
    systemMonitoring = new SystemMonitoring();
    systemMonitoring->start();
}
