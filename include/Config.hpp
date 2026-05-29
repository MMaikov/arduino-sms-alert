#pragma once

#include <Arduino.h>

constexpr unsigned long TERMINAL_BAUD_RATE = 9600;

constexpr uint8_t SMS_MODULE_RX = 7;
constexpr uint8_t SMS_MODULE_TX = 8;
constexpr uint8_t SMS_MODULE_PWRKEY = 12;
constexpr long SMS_MODULE_BAUD_RATE = 9600;
constexpr unsigned long SMS_TIMEOUT_MS = 5000;

constexpr float ALPHA_HP = 0.995f;
constexpr float ALPHA_LP = 0.999f;
constexpr float ADC_MAX_COUNT = 1024.0f;
constexpr float BURDEN_RESISTOR = 100.0;
constexpr float CT_RATIO = 500.0f;
constexpr float VCC = 5.18f;
constexpr uint8_t NUM_CT = 2;

// 40 characters + null terminator
constexpr uint8_t INPUT_BUFFER_SIZE = 41;

constexpr uint8_t MAX_PHONE_NUMBERS = 5; // 255th value reserved as not existing
constexpr uint8_t PHONE_NUMBER_BUF_SIZE = 16;

constexpr uint8_t EEPROM_NUM_PHONES_ADDRESS = 0;
constexpr uint8_t EEPROM_PHONES_ADDRESS = EEPROM_NUM_PHONES_ADDRESS + 1; // 16 bytes, max 5 so in total 4065
constexpr uint8_t EEPROM_THRESHOLDS_ADDRESS = EEPROM_PHONES_ADDRESS + (MAX_PHONE_NUMBERS*PHONE_NUMBER_BUF_SIZE); // 16 bytes, max 8 so in total 128 bytes

static_assert(EEPROM_THRESHOLDS_ADDRESS < 1024, "EEPROM capacity exceeded");

using FlashString = __FlashStringHelper;

enum class BODLevel : uint8_t {
    V4_3     = 0b100,
    V2_7     = 0b101,
    V1_8     = 0b110,
    Disabled = 0b111
};

enum class BootSize : uint8_t {
    Words_256  = 0b11,
    Words_512  = 0b10,
    Words_1024 = 0b01,
    Words_2048 = 0b00
};

enum class ClockSource : uint8_t {
    External_Clock         = 0b0000,
    Internal_8MHz          = 0b0010,
    Internal_128kHz        = 0b0011,
    Ext_Crystal_LowPower   = 0b1111,
    Ext_Crystal_FullSwing  = 0b0111
};

enum class StartupTime
{
    // Common for Internal RC and External Clock
    Cycles_6_0ms,         // 6 Clock cycles, no delay
    Cycles_6_4ms,         // 6 Clock cycles, 4.1ms delay
    Cycles_6_65ms,        // 6 Clock cycles, 65ms delay
    
    // Common for Ceramic Resonators
    Cycles_258_4ms,       // 258 Clock cycles, 4.1ms delay
    Cycles_258_65ms,      // 258 Clock cycles, 65ms delay
    
    // Common for Crystals
    Cycles_1K_0ms,        // 1,000 Clock cycles, no delay
    Cycles_1K_4ms,        // 1,000 Clock cycles, 4.1ms delay
    Cycles_1K_65ms,       // 1,000 Clock cycles, 65ms delay
    Cycles_16K_0ms,       // 16,000 Clock cycles, no delay
    Cycles_16K_4ms,       // 16,000 Clock cycles, 4.1ms delay
    Cycles_16K_65ms,      // 16,000 Clock cycles, 65ms delay (Arduino Uno Default)

    // Low Frequency Crystal (32.768kHz)
    Cycles_1K_32ms,       // 1,000 Clock cycles, 32ms delay
    Cycles_32K_65ms,      // 32,000 Clock cycles, 65ms delay

    Reserved,             // Bit patterns marked as reserved in datasheet
    Invalid               // Logic error/fallback
};

enum class MemoryLock : uint8_t {
    None = 0b11,
    NoWrite = 0b10,
    NoReadWrite = 0b00
};

enum class SectionLock : uint8_t {
    None = 0b11,
    NoWrite = 0b10,
    NoReadWrite = 0b00,
    NoRead = 0b01
};

struct FuseReport {
    // Low Byte Info
    bool      clockDivide8;
    bool      outputClockOnPortB0;
    ClockSource clockSource;
    StartupTime startupTime;

    // High Byte Info
    bool      externalResetDisabled; // RSTDISBL
    bool      debugWireEnabled;      // DWEN
    bool      spiEnabled;            // SPIEN
    bool      watchdogAlwaysOn;      // WDTON
    bool      preserveEeprom;        // EESAVE
    BootSize  bootSize;
    bool      bootToLoader;          // BOOTRST

    // Extended Byte Info
    BODLevel  bod;

    MemoryLock chipLock;
    SectionLock appSection;
    SectionLock bootSection;

    uint8_t lowFuse;
    uint8_t highFuse;
    uint8_t extFuse;
    uint8_t lockBits;
};

struct ThresholdRange {
    float lower = NAN;
    float upper = NAN;

    constexpr ThresholdRange() {}
    constexpr ThresholdRange(float lower, float upper) : lower(lower), upper(upper) {}

    bool empty() const {
        return !isfinite(lower) && !isfinite(upper);
    }
};

static_assert(NUM_CT <= 8, "Cannot have more than 8 CTs");

class Config
{
public:
    static uint8_t getNumberOfPhones();
	static const char* getPhoneNumber(int slot);
    static ThresholdRange* getThresholds();

    static void setNumberOfPhones(uint8_t number);
	static void setPhoneNumber(int slot, const char* number);
    static void setThreshold(uint8_t index, const ThresholdRange& threshold);

    static FuseReport getFuseReport();
private:
    static FuseReport parseFuses(uint8_t low, uint8_t high, uint8_t ext, uint8_t lock);
    static StartupTime getStartupTime(ClockSource src, uint8_t lowFuse);
private:
    static uint8_t s_NumPhones;
	static char s_PhoneNumbers[MAX_PHONE_NUMBERS][PHONE_NUMBER_BUF_SIZE];
    static ThresholdRange s_Thresholds[NUM_CT];
};