#pragma once

class OtaUpdate
{
public:
    virtual void getBinary(String binaryName, String hostName);
    virtual void updateFirmware(int contentLength);
};