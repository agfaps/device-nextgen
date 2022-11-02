#include <Arduino.h>
#include <unity.h>

#include "MessageProcessor/MessageProcessor.h"

void setUp(void)
{
    // init MessageProcessor
    auto &messageProcessor = MessageProcessor::getSingleInstance();
    messageProcessor.start();
}

void tearDown(void)
{

}

void test_receive_mqtt_ota_msg()
{
    String message;
    auto &messageProcessor = MessageProcessor::getSingleInstance();
    
    while (true)
    {
        message = messageProcessor.getMessage(String("/update/url/"));
        if (message != "false")
        {
            // means we have received message from topic: "/update/url/"
            // and we check if sender send: "hostname/update.bin"
            break;
        }
    }
    TEST_ASSERT_EQUAL_STRING("hostname/update.bin", message.c_str());
}

void setup()
{
    delay(2000);

    UNITY_BEGIN();
    RUN_TEST(test_receive_mqtt_ota_msg);
    
    UNITY_END();
}

void loop()
{
  // put your main code here, to run repeatedly:
  vTaskDelay(pdMS_TO_TICKS(2000));
  yield();
}