#pragma once

#include <string>

class MessageProcessor
{
private:
    MessageProcessor();

public:
    // Meyer's Singleton
    MessageProcessor(const MessageProcessor &) = delete;
    MessageProcessor &operator=(const MessageProcessor &) = delete;
    static MessageProcessor &getSingleInstance();
    ~MessageProcessor();

    void start();
    String getMessage(String topic);
    void sendMessage(String topic, String message);
};