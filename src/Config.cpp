#include "Config.hpp"

#include <EEPROM.h>

char Config::s_PhoneNumber[PHONE_NUMBER_BUF_SIZE] = {0};

const char* Config::getPhoneNumber() {
    if (s_PhoneNumber[0] == '\0') {
        for (size_t i = 0; i < PHONE_NUMBER_BUF_SIZE-1; i++) {
            s_PhoneNumber[i] = EEPROM.read(EEPROM_PHONE_NUMBER_ADDRESS + i);
            if ((uint8_t)s_PhoneNumber[i] == 0xff) {
                s_PhoneNumber[i] = '\0';
                break;
            }
        }
        s_PhoneNumber[PHONE_NUMBER_BUF_SIZE-1] = '\0';
    }
    return s_PhoneNumber;
}

void Config::setPhoneNumber(const char* number) {
    size_t i;
    for (i = 0; i < PHONE_NUMBER_BUF_SIZE-1 && number[i] != '\0'; i++) {
        s_PhoneNumber[i] = number[i];
        EEPROM.update(EEPROM_PHONE_NUMBER_ADDRESS + i, number[i]);
    }
    s_PhoneNumber[i] = '\0';
    EEPROM.update(EEPROM_PHONE_NUMBER_ADDRESS + i, '\0');
}