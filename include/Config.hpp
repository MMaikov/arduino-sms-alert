#pragma once

#include <Arduino.h>

#define TERMINAL_BAUD_RATE 9600

#define SMS_MODULE_RX 7
#define SMS_MODULE_TX 8
#define SMS_MODULE_PWRKEY 12
#define SMS_MODULE_BAUD_RATE 9600
#define SMS_TIMEOUT_MS 5000

// 40 characters + null terminator
#define INPUT_BUFFER_SIZE 41

#define PHONE_NUMBER_BUF_SIZE 16
#define EEPROM_PHONE_NUMBER_ADDRESS 0 // 16 bytes

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

class Config
{
public:
	static const char* getPhoneNumber();

	static void setPhoneNumber(const char* number);

    static FuseReport getFuseReport();
private:
    static FuseReport parseFuses(uint8_t low, uint8_t high, uint8_t ext, uint8_t lock);
    static StartupTime getStartupTime(ClockSource src, uint8_t lowFuse);
private:
	static char s_PhoneNumber[PHONE_NUMBER_BUF_SIZE];
};