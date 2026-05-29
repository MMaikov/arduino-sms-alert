#pragma once

#include <Arduino.h>

#include "Config.hpp"
#include "SMS.hpp"
#include "CurrentTransformer.hpp"

enum class DeviceState {
    On, Off
};

class Application
{
public:
    Application();

    void Init();
    void Update();
private:
    void HandleSerialCommands();
    bool ProcessLine(const char* line);

    size_t BeginsWith(const char* line, const char* prefix_P);

    void PrintSystemInfo();
private:
    SMS m_SMS;   
    CurrentTransformer m_CT[NUM_CT];
    DeviceState m_DeviceStates[NUM_CT];
    
    uint8_t m_InputBufferPtr = 0;
    char m_InputBuffer[INPUT_BUFFER_SIZE];
};