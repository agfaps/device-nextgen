#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <unordered_map>

#include "MessageProcessor.h"

const char* ssid = "Nama SSID Gateway/AP yang dipakai";
const char* password = "passwordnyaapa";

//const char* mqtt_server = "192.168.1.144";
const char* mqtt_server = "MQTT_BROKER_IP_ADDRESS";

WiFiClient espWiFiClient;
PubSubClient mqttPubSubClient(espWiFiClient);
std::unordered_map<std::string, std::string> mqttMessage{};

static SemaphoreHandle_t bin_sem;
static const char* TAG = "MessageProcessor";

static TickType_t xLastWakeTime;

static uint32_t intervalMessageProcessorThread = 1000; // run every second


void callback(char* topic, byte* message, unsigned int length);
void messageProcessorThread(void *pvParameters);
void registerMessage(String topic, String message);
String retrieveMessage(String topic);


MessageProcessor::MessageProcessor()
{}

MessageProcessor::~MessageProcessor()
{}

MessageProcessor &MessageProcessor::getSingleInstance()
{
    //Meyer's singleton 
    static MessageProcessor instance ;
    return instance ;
}

void MessageProcessor::start()
{
    bin_sem = xSemaphoreCreateMutex();

    delay(100);
    // connect to a WiFi network
    ESP_LOGI(TAG, "Connecting to ");
    ESP_LOGI(TAG, "%s", ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        ESP_LOGI(TAG, ".");
    }
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "WiFi connected");
    ESP_LOGI(TAG, "IP address: ");
    ESP_LOGI(TAG, "%s", WiFi.localIP().toString().c_str());

    // setup mqtt pubsub client
    mqttPubSubClient.setServer(mqtt_server, 1883);
    mqttPubSubClient.setCallback(callback);

    xTaskCreatePinnedToCore(
        messageProcessorThread,
        "messageProcessorThread",
        4096,
        NULL,
        10,
        NULL,
        0
    );
}

void callback(char* topic, byte* message, unsigned int length)
{
    message[length] = '\0';
    ESP_LOGI(TAG, "Message arrived on topic: %s . Message: %s", topic, (char *)message);
    String _message = String((char*)message);
    String _topic = String(topic);

    if (_topic.equals("/update/url/") == 1)
    {
        // store topic and message as pair in an unordered map
        registerMessage(_topic, _message);
    }
    // else if ()
    // {
    //     // process another topic
    // }
}

void reconnect()
{
    // Loop until we're reconnected
    while (!mqttPubSubClient.connected())
    {
        ESP_LOGI(TAG, "Attempting MQTT connection...");
        // Attempt to connect
        if (mqttPubSubClient.connect("ESP32Client"))
        {
            ESP_LOGI(TAG, "connected");
            // Subscribe to update url topic
            mqttPubSubClient.subscribe("/update/url/");
        }
        else
        {
            ESP_LOGI(TAG, "failed, rc = %d  try again in 5 seconds", mqttPubSubClient.state());
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void messageProcessorThread(void *pvParameters)
{
    ESP_LOGI(TAG, "messageProcessorThread running on core %d", xPortGetCoreID());

    // We do this only once 
    xLastWakeTime = xTaskGetTickCount();

    while (true)
    {
        if (!mqttPubSubClient.connected())
        {
            reconnect();
        }
        mqttPubSubClient.loop();
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS( intervalMessageProcessorThread ));
    }
}

void registerMessage(String topic, String message)
{
    if (xSemaphoreTake(bin_sem, portMAX_DELAY) == pdTRUE)
    {
        mqttMessage.insert(std::make_pair(std::string(topic.c_str()), std::string(message.c_str())));
        xSemaphoreGive(bin_sem);
    }
}

String retrieveMessage(String topic)
{
    if (xSemaphoreTake(bin_sem, portMAX_DELAY) == pdTRUE)
    {
        auto it = mqttMessage.find(std::string(topic.c_str()));
        if (it != mqttMessage.end())
        {
            // retrieve
            std::string tmp = mqttMessage.at(topic.c_str());
            // deregister
            mqttMessage.erase(it);
            
            xSemaphoreGive(bin_sem);
            return String(tmp.c_str());
        }
        else
        {
            xSemaphoreGive(bin_sem);
            return String("false");
        }
        xSemaphoreGive(bin_sem);
    }
    return String("false");
}

String MessageProcessor::getMessage(String topic)
{
    retrieveMessage(topic);
}

void MessageProcessor::sendMessage(String topic, String message)
{
    // directly send mqtt message
    mqttPubSubClient.publish(topic.c_str(), message.c_str());
}
