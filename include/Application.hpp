#pragma once

#include <Arduino.h>

#include "Config.hpp"
#include "SMS.hpp"

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
    
    uint8_t m_InputBufferPtr = 0;
    char m_InputBuffer[INPUT_BUFFER_SIZE];
};