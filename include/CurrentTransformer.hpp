#pragma once

#include <Arduino.h>

#include "Config.hpp"

class CurrentTransformer {
public:
    CurrentTransformer(uint8_t pin);

    void Init();
    void Update();

    float readCurrent() const;
    float getSquaredCount() const;

    static float CalculateSquaredCount(float current);
    static float FromSquaredCount(float sq);
private:
    const uint8_t m_Pin;

    float m_FilteredSignal = 0.0f;
    float m_LastSample = 0.0f;
    float m_SqSignalFiltered = 0.0f;
};