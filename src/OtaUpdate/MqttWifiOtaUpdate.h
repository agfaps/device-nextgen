#pragma once
#include "OtaUpdate.h"

class MqttWifiOtaUpdate : public OtaUpdate
{
public:
    MqttWifiOtaUpdate();
    ~MqttWifiOtaUpdate() = default;
    void start();
    void getBinary(String binaryName, String hostName);
    void updateFirmware(int contentLength);
};