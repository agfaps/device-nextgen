#include <Arduino.h>
#include "SystemMonitoring.h"
#include "MessageProcessor/MessageProcessor.h"

static const char* TAG = "SystemMonitoring";

static TickType_t xLastWakeTime;
static uint32_t intervalsystemMonitoringThread = 5000; // run every 5 second

void systemMonitoringThread(void *pvParameters);

void SystemMonitoring::start()
{
    // create thread to check message availability on topic /update/url/
    xTaskCreatePinnedToCore(
        systemMonitoringThread,
        "systemMonitoringThread",
        4096,
        NULL,
        10,
        NULL,
        0
    );
}

bool getEnclosureOpenedStatus()
{
    // assume we are reading enclosure status if opened while
    // operating without maintainer intention (ex: burglar/vandalism)
    return false;
}

void systemMonitoringThread(void *pvParameters)
{
    ESP_LOGI(TAG, "systemMonitoringThread running on core %d", xPortGetCoreID());

    // We do this only once 
    xLastWakeTime = xTaskGetTickCount();

    auto &messageProcessor = MessageProcessor::getSingleInstance();

    String message;

    while (true)
    {
        // report enclosure opened status to server
        if (true == getEnclosureOpenedStatus())
        {
            messageProcessor.sendMessage("device/enclosure", "opened");
        }
        else
        {
            messageProcessor.sendMessage("device/enclosure", "closed");
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS( intervalsystemMonitoringThread ));
    }
}