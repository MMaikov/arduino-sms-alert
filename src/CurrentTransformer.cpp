#include "CurrentTransformer.hpp"

CurrentTransformer::CurrentTransformer(uint8_t pin)
    : m_Pin(pin)
{
}

void CurrentTransformer::Init() {
    m_LastSample = analogRead(m_Pin);
}

void CurrentTransformer::Update() {
    const int rawSample = analogRead(m_Pin);

    m_FilteredSignal = ALPHA_HP * (m_FilteredSignal + rawSample - m_LastSample);
    m_LastSample = rawSample;

    const float sqSignal = m_FilteredSignal * m_FilteredSignal;
    m_SqSignalFiltered = (ALPHA_LP * m_SqSignalFiltered) + ((1.0 - ALPHA_LP) * sqSignal);
}

float CurrentTransformer::readCurrent() const
{
    return FromSquaredCount(m_SqSignalFiltered);
}

float CurrentTransformer::getSquaredCount() const
{
    return m_SqSignalFiltered;
}

float CurrentTransformer::CalculateSquaredCount(float current)
{
    // Reverse of what is in CurrentTransformer::FromSquaredCount
    return (current*current*BURDEN_RESISTOR*BURDEN_RESISTOR*ADC_MAX_COUNT*ADC_MAX_COUNT)/(CT_RATIO*CT_RATIO*VCC*VCC);
}

float CurrentTransformer::FromSquaredCount(float sq)
{
    const float rmsADC = sqrtf(sq);
	const float rmsVoltage = rmsADC * (VCC / ADC_MAX_COUNT); 
	const float rmsCurrent = rmsVoltage / BURDEN_RESISTOR * CT_RATIO;
    return rmsCurrent;
}