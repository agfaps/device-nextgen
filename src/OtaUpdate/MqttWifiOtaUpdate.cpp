#include <stdbool.h>
#include <Arduino.h>
#include <WiFi.h>
#include <Update.h>
#include "MqttWifiOtaUpdate.h"
#include "MessageProcessor/MessageProcessor.h"

#define PORTNUM 80

extern WiFiClient espWiFiClient;

static OtaUpdate *instance;
static const char* TAG = "MqttWifiOtaUpdate";

static TickType_t xLastWakeTime;

static uint32_t intervalMqttWifiOtaUpdateThread = 5000; // run every 5 second

int contentLength = 0;
bool isValidContentType = false;

void mqttWifiOtaUpdateThread(void *pvParameters);

MqttWifiOtaUpdate::MqttWifiOtaUpdate()
{
    instance = this;
}

void MqttWifiOtaUpdate::start()
{
    // create thread to check message availability on topic /update/url/
    xTaskCreatePinnedToCore(
        mqttWifiOtaUpdateThread,
        "mqttWifiOtaUpdateThread",
        4096,
        NULL,
        10,
        NULL,
        0
    );
}

String getHeaderValue(String header, String headerName)
{
    return header.substring(strlen(headerName.c_str()));
}

void MqttWifiOtaUpdate::getBinary(String binaryName, String hostName)
{
    ESP_LOGI(TAG, "Connecting to: %s", hostName.c_str());
    if (espWiFiClient.connect(hostName.c_str(), PORTNUM))
    {
        // Connection Succeed.
        // Fecthing the bin
        ESP_LOGI(TAG, "Fetching Bin: %s", binaryName.c_str());

        // Get the contents of the bin file
        espWiFiClient.print(String("GET ") + binaryName + " HTTP/1.1\r\n" +
                        "Host: " + hostName + "\r\n" +
                        "Cache-Control: no-cache\r\n" +
                        "Connection: close\r\n\r\n");

        unsigned long timeout = millis();

        while (espWiFiClient.available() == 0)
        {
            if (millis() - timeout > 5000)
            {
                ESP_LOGI(TAG, "espWiFiClient Timeout !");
                espWiFiClient.stop();
                return;
            }
        }
        while (espWiFiClient.available())
        {
            // read line till /n
            String line = espWiFiClient.readStringUntil('\n');
            // remove space, to check if the line is end of headers
            line.trim();

            // if the the line is empty,
            // this is end of headers
            // break the while and feed the
            // remaining `client` to the
            // Update.writeStream();
            if (!line.length())
            {
                //headers ended
                break; // and get the OTA started
            }

            // Check if the HTTP Response is 200
            // else break and Exit Update
            if (line.startsWith("HTTP/1.1"))
            {
                if (line.indexOf("200") < 0)
                {
                    ESP_LOGI(TAG, "Got a non 200 status code from server. Exiting OTA Update.");
                    break;
                }
            }

            // extract headers here
            // Start with content length
            if (line.startsWith("Content-Length: "))
            {
                contentLength = atoi((getHeaderValue(line, "Content-Length: ")).c_str());
                ESP_LOGI(TAG, "Got %d bytes from server", contentLength);
            }

            // Next, the content type
            if (line.startsWith("Content-Type: "))
            {
                String contentType = getHeaderValue(line, "Content-Type: ");
                ESP_LOGI(TAG, "Got %d  payload.", contentType);
                if (contentType == "application/octet-stream")
                {
                    isValidContentType = true;
                }
            }
        }
    }
    else
    {
        // Connect to S3 failed
        // Probably a choppy network?
        ESP_LOGI(TAG, "Connection to %s failed. Please check your setup", hostName.c_str());
        // retry??
    }

    // Check what is the contentLength and if content type is `application/octet-stream`
    ESP_LOGI(TAG, "contentLength : %d, isValidContentType : %s", contentLength, isValidContentType ?  "true" : "false");

    // check contentLength and content type
    if (contentLength && isValidContentType)
    {
        this->updateFirmware(contentLength);
    }
    else
    {
        ESP_LOGI(TAG, "There was no content in the response");
        espWiFiClient.flush();
    }
}

void MqttWifiOtaUpdate::updateFirmware(int contentLength)
{
    // Check if there is enough to OTA Update
    bool canBegin = Update.begin(contentLength);
    if (canBegin)
    {
        ESP_LOGI(TAG, "Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!");
        size_t written = Update.writeStream(espWiFiClient);

        if (written == contentLength)
        {
            ESP_LOGI(TAG, "Written : %d successfully", (int)written);
        }
        else
        {
            Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?" );
            ESP_LOGI(TAG, "Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!");
            // retry??
        }

        if (Update.end())
        {
            ESP_LOGI(TAG, "OTA done!");
            if (Update.isFinished())
            {
                ESP_LOGI(TAG, "Update successfully completed. Rebooting.");
                ESP.restart();
            }
            else
            {
                ESP_LOGI(TAG, "Update not finished? Something went wrong!");
            }
        }
        else
        {
            ESP_LOGI(TAG, "Error Occurred. Error #: %d", Update.getError());
        }
    }
    else
    {
        // not enough space to begin OTA
        // Understand the partitions and
        // space availability
        ESP_LOGI(TAG, "Not enough space to begin OTA");
        espWiFiClient.flush();
    }
}

String getBinaryName(String url)
{
    int index = 0;

    // Search for last /
    // "hostname/update.bin"
    for (int i = 0; i < url.length(); i++)
    {
        if (url[i] == '/')
        {
            index = i;
        }
    }

    String binaryName = "";

    // Create binaryName
    for (int i = index; i < url.length(); i++)
    {
        binaryName += url[i];
    }

    return binaryName;
}

String getHostName(String url)
{
    int index = 0;

    // Search for last /
    // "hostname/update.bin"
    for (int i = 0; i < url.length(); i++) {
        if (url[i] == '/') {
            index = i;
        }
    }

    String hostName = "";

    // Create binName
    for (int i = 0; i < index; i++) {
        hostName += url[i];
    }

    return hostName;
}

void mqttWifiOtaUpdateThread(void *pvParameters)
{
    ESP_LOGI(TAG, "mqttWifiOtaUpdateThread running on core %d", xPortGetCoreID());

    // We do this only once 
    xLastWakeTime = xTaskGetTickCount();

    auto &messageProcessor = MessageProcessor::getSingleInstance();

    String message;

    while (true)
    {
        message = messageProcessor.getMessage(String("/update/url/"));
        if (message != "false")
        {
            instance->getBinary(getBinaryName(message), getHostName(message));
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS( intervalMqttWifiOtaUpdateThread ));
    }
}