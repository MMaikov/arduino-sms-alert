#include "Config.hpp"

#include <EEPROM.h>

#include <avr/boot.h>

uint8_t Config::s_NumPhones = 0xff;
char Config::s_PhoneNumbers[MAX_PHONE_NUMBERS][PHONE_NUMBER_BUF_SIZE] = {0};
ThresholdRange Config::s_Thresholds[NUM_CT] = {};

uint8_t Config::getNumberOfPhones() {
    if (s_NumPhones == 0xff) {
        s_NumPhones = EEPROM.read(EEPROM_NUM_PHONES_ADDRESS);
    }
    return s_NumPhones;
}

const char* Config::getPhoneNumber(int slot) {
    if (slot < 0 || slot >= MAX_PHONE_NUMBERS) return NULL;

    if (s_PhoneNumbers[slot][0] == '\0') {
        for (size_t i = 0; i < PHONE_NUMBER_BUF_SIZE-1; i++) {
            s_PhoneNumbers[slot][i] = EEPROM.read((EEPROM_PHONES_ADDRESS + slot*PHONE_NUMBER_BUF_SIZE) + i);
            if ((uint8_t)s_PhoneNumbers[slot][i] == 0xff) {
                s_PhoneNumbers[slot][i] = '\0';
                break;
            }
        }
        s_PhoneNumbers[slot][PHONE_NUMBER_BUF_SIZE-1] = '\0';
    }
    return s_PhoneNumbers[slot];
}

ThresholdRange* Config::getThresholds() {
    if (s_Thresholds[0].empty()) {
        int start = EEPROM_THRESHOLDS_ADDRESS;
        for (int i = 0; i < NUM_CT; i++) {
            EEPROM.get(start, s_Thresholds[i]);
            start += sizeof(ThresholdRange);
        }
    }
    return s_Thresholds;
}

void Config::setNumberOfPhones(uint8_t number) {
    if (number >= MAX_PHONE_NUMBERS) return;

    s_NumPhones = number;
    EEPROM.update(EEPROM_NUM_PHONES_ADDRESS, number);
}

void Config::setPhoneNumber(int slot, const char* number) {
    if (slot < 0 || slot >= MAX_PHONE_NUMBERS) return;

    size_t i;
    for (i = 0; i < PHONE_NUMBER_BUF_SIZE-1 && number[i] != '\0'; i++) {
        s_PhoneNumbers[slot][i] = number[i];
        EEPROM.update((EEPROM_PHONES_ADDRESS + slot*PHONE_NUMBER_BUF_SIZE) + i, number[i]);
    }
    s_PhoneNumbers[slot][i] = '\0';
    EEPROM.update((EEPROM_PHONES_ADDRESS + slot*PHONE_NUMBER_BUF_SIZE) + i, '\0');
}

void Config::setThreshold(uint8_t index, const ThresholdRange& threshold) {
    if (index >= NUM_CT) return;

    s_Thresholds[index] = threshold;
    EEPROM.put(EEPROM_THRESHOLDS_ADDRESS + (index * sizeof(ThresholdRange)), s_Thresholds[index]);
}

FuseReport Config::getFuseReport()
{
    uint8_t lowFuse = boot_lock_fuse_bits_get(GET_LOW_FUSE_BITS);
    uint8_t highFuse = boot_lock_fuse_bits_get(GET_HIGH_FUSE_BITS);
    uint8_t extFuse = boot_lock_fuse_bits_get(GET_EXTENDED_FUSE_BITS);
    uint8_t lockBits = boot_lock_fuse_bits_get(GET_LOCK_BITS);
    return parseFuses(lowFuse, highFuse, extFuse, lockBits);
}

FuseReport Config::parseFuses(uint8_t low, uint8_t high, uint8_t ext, uint8_t lock)
{
    FuseReport report;

    report.lowFuse = low;
    report.highFuse = high;
    report.extFuse = ext;
    report.lockBits = lock;

    // Inverting logic because 0 = Programmed/True
    report.clockDivide8        = !(low & (1 << 7));
    report.outputClockOnPortB0 = !(low & (1 << 6));
    report.clockSource         = static_cast<ClockSource>(low & 0x0F);
    report.startupTime         = getStartupTime(report.clockSource, low);

    report.externalResetDisabled = !(high & (1 << 7));
    report.debugWireEnabled      = !(high & (1 << 6));
    report.spiEnabled            = !(high & (1 << 5));
    report.watchdogAlwaysOn      = !(high & (1 << 4));
    report.preserveEeprom        = !(high & (1 << 3));
    report.bootSize              = static_cast<BootSize>((high >> 1) & 0x03);
    report.bootToLoader          = !(high & (1 << 0));

    report.bod = static_cast<BODLevel>(ext & 0x07);

    report.chipLock    = static_cast<MemoryLock>(lock & 0x03);
    report.appSection  = static_cast<SectionLock>((lock >> 2) & 0x03);
    report.bootSection = static_cast<SectionLock>((lock >> 4) & 0x03);

    return report;
}

StartupTime Config::getStartupTime(ClockSource src, uint8_t lowFuse) 
{
    uint8_t sutBits = (lowFuse >> 4) & 0x03;

    switch (src) {
        case ClockSource::Ext_Crystal_LowPower:
        case ClockSource::Ext_Crystal_FullSwing:
            if (sutBits == 0b00) return StartupTime::Cycles_258_4ms;
            if (sutBits == 0b01) return StartupTime::Cycles_258_65ms;
            if (sutBits == 0b10) return StartupTime::Cycles_1K_0ms;
            return StartupTime::Cycles_16K_65ms; // 0b11

        case ClockSource::Internal_8MHz:
        case ClockSource::External_Clock:
            if (sutBits == 0b00) return StartupTime::Cycles_6_0ms;
            if (sutBits == 0b01) return StartupTime::Cycles_6_4ms;
            if (sutBits == 0b10) return StartupTime::Cycles_6_65ms;
            return StartupTime::Reserved;

        case ClockSource::Internal_128kHz:
            if (sutBits == 0b00) return StartupTime::Cycles_6_0ms;
            if (sutBits == 0b01) return StartupTime::Cycles_6_4ms;
            if (sutBits == 0b10) return StartupTime::Cycles_6_65ms;
            return StartupTime::Reserved;

        default:
            return StartupTime::Invalid;
    }
}