#pragma once

#include <Arduino.h>
#include <SoftwareSerial.h>

#include "Config.hpp"

struct Message
{
	const char* phoneNumber;
	const FlashString* message;
	Message()
		: phoneNumber(nullptr), message(nullptr)
	{}
	Message(const char* number, const FlashString* message)
		: phoneNumber(number), message(message)
	{}
};

enum class ModemStatus
{
	Ready,
	SendSMS,
	SendCMD,
	WaitForPrompt,
	WaitForOk,
    FinishError
};

class SMS
{
public:
	SMS(uint8_t rx, uint8_t tx);

	void begin(long speed);

	bool sendSMS(const Message& message, bool priority=false);

	bool sendCMD(const char* command);

	void setWriteToSerial(bool status);

	void update();
private:
	SoftwareSerial m_SmsSerial;
	Message m_Message;
	const char* m_Command;
	ModemStatus m_Status;
	unsigned long m_LastTime;
	bool m_WriteToSerial = false;
    char m_CurrentChar;
	char m_PreviousChar;
};