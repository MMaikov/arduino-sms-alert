#pragma once

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

class Config
{
public:
	static const char* getPhoneNumber();

	static void setPhoneNumber(const char* number);
private:
	static char s_PhoneNumber[PHONE_NUMBER_BUF_SIZE];
};