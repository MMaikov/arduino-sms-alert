#include <Arduino.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

#define SMS_MODULE_RX 7
#define SMS_MODULE_TX 8
#define SMS_MODULE_BAUD_RATE 9600
#define SMS_TIMEOUT_MS 5000

// 40 characters + null terminator
#define INPUT_BUFFER_SIZE 41

#define PHONE_NUMBER_BUF_SIZE 16
#define EEPROM_PHONE_NUMBER_ADDRESS 0 // 16 bytes

static uint8_t input_buf_ptr = 0;
static char input_buf[INPUT_BUFFER_SIZE];

struct Message
{
	const char* phoneNumber;
	const __FlashStringHelper* message;
	Message()
		: phoneNumber(nullptr), message(nullptr)
	{}
	Message(const char* number, const __FlashStringHelper* message)
		: phoneNumber(number), message(message)
	{}
};

enum class ModemStatus
{
	Ready,
	SendSMS,
	SendCMD,
	WaitForPrompt,
	WaitForOk
};

class SMS
{
public:
	SMS(uint8_t rx, uint8_t tx)
		: m_SmsSerial(rx, tx), m_Command(nullptr), m_Status(ModemStatus::Ready), m_LastTime(millis())
	{
	}

	void begin(long speed) {
		m_SmsSerial.begin(speed);
	}

	bool sendSMS(const Message& message, bool priority=false) {
		if (priority) {
			unsigned long startWait = millis();
			while (m_Status != ModemStatus::Ready) {
				update();
				if (millis() - startWait >= SMS_TIMEOUT_MS) break;
			}
		}

		if (priority && m_Status != ModemStatus::Ready) {
			// PANIC
			while(true) {
				digitalWrite(LED_BUILTIN, HIGH);
				delay(100);
				digitalWrite(LED_BUILTIN, LOW);
				delay(100);
			}
		}

		if (m_Status == ModemStatus::Ready) {
			m_Message = message;
			m_Status = ModemStatus::SendSMS;
			m_LastTime = millis();
			return true;
		}
		return false;
	}

	bool sendCMD(const char* command) {
		if (m_Status == ModemStatus::Ready) {
			m_Command = command;
			m_Status = ModemStatus::SendCMD;
			m_LastTime = millis();
			return true;
		}
		return false;
	}

	void setWriteToSerial(bool status) {
		m_WriteToSerial = status;
	}

	void update() {
		
		if (m_Status == ModemStatus::SendSMS) {
			m_SmsSerial.print(F("AT+CMGS=\""));
			m_SmsSerial.print(m_Message.phoneNumber);
			m_SmsSerial.println('\"');
			m_Status = ModemStatus::WaitForPrompt;
			m_LastTime = millis();
		} else if (m_Status == ModemStatus::SendCMD) {
			m_SmsSerial.println(m_Command);
			m_Status = ModemStatus::WaitForOk;
			m_LastTime = millis();
		}

		const char match[] = "OK";
		while (m_SmsSerial.available() > 0) {
			char currentChar = m_SmsSerial.read();
			if (m_WriteToSerial) {
				Serial.write(currentChar);
			}

			if (currentChar == '>') {
				if (m_Status == ModemStatus::WaitForPrompt) {
					m_SmsSerial.print(m_Message.message);
					m_SmsSerial.write(0x1A);
					m_Status = ModemStatus::WaitForOk;
					m_LastTime = millis();
				} else {
					m_SmsSerial.print(F("Test MSG"));
					m_SmsSerial.write(0x1A);
				}
			}

			if (currentChar == match[m_MatchIdx]) {
				m_MatchIdx++;
				if (match[m_MatchIdx] == '\0') {
					if (m_Status == ModemStatus::WaitForOk) {
						m_Status = ModemStatus::Ready;
						m_LastTime = millis();
						if (m_WriteToSerial) {
							m_WriteToSerial = false;
							Serial.print(F("\r\n> "));
						}
					}
					m_MatchIdx = 0;
				}
			} else {
				m_MatchIdx = 0;
			}
		}

		if (m_Status != ModemStatus::Ready && (millis() - m_LastTime > SMS_TIMEOUT_MS)) {
			m_Status = ModemStatus::Ready; // Reset;
			if (m_WriteToSerial) {
				m_WriteToSerial = false;
				Serial.println(F("\r\n[Timeout]> "));
			}
		}
	}

	SoftwareSerial& getSerial() {
		return m_SmsSerial;
	}
private:
	SoftwareSerial m_SmsSerial;
	Message m_Message;
	const char* m_Command;
	ModemStatus m_Status;
	unsigned long m_LastTime;
	bool m_WriteToSerial = false;
	int m_MatchIdx = 0;
};

class Config
{
public:
	static const char* getPhoneNumber() {
		if (s_PhoneNumber[0] == '\0') {
			for (size_t i = 0; i < PHONE_NUMBER_BUF_SIZE-1; i++) {
				s_PhoneNumber[i] = EEPROM.read(EEPROM_PHONE_NUMBER_ADDRESS + i);
				if (s_PhoneNumber[i] == 0xff) {
					s_PhoneNumber[i] = '\0';
					break;
				}
			}
			s_PhoneNumber[PHONE_NUMBER_BUF_SIZE-1] = '\0';
		}
		return s_PhoneNumber;
	}

	static void setPhoneNumber(const char* number) {
		size_t i;
		for (i = 0; i < PHONE_NUMBER_BUF_SIZE-1 && number[i] != '\0'; i++) {
			s_PhoneNumber[i] = number[i];
			EEPROM.update(EEPROM_PHONE_NUMBER_ADDRESS + i, number[i]);
		}
		s_PhoneNumber[i] = '\0';
		EEPROM.update(EEPROM_PHONE_NUMBER_ADDRESS + i, '\0');
	}
private:
	static char s_PhoneNumber[PHONE_NUMBER_BUF_SIZE];
};

char Config::s_PhoneNumber[PHONE_NUMBER_BUF_SIZE] = {0};

static SMS sms(SMS_MODULE_RX, SMS_MODULE_TX);

static void handle_serial_commands();
static bool process_line(const char* line);

void setup() {
	pinMode(LED_BUILTIN, OUTPUT);

	Serial.begin(9600);
	Serial.println(F("\r\nArduino SMS Alert system"));
	Serial.print(F("> "));

	sms.begin(SMS_MODULE_BAUD_RATE);
}

void loop() {
	sms.update();

	handle_serial_commands();
}

static void handle_serial_commands() {
	while (Serial.available() > 0) {
		char c = Serial.read();
		Serial.write(c); // Echo
		if (c == '\b' || c == 0x7f) { // Backspace
			if (input_buf_ptr > 0) {
				input_buf_ptr--;
			}
		} else if (c == '\n') {
			if (input_buf_ptr >= INPUT_BUFFER_SIZE) {
				Serial.print(F("input discarded, max only "));
				Serial.print(INPUT_BUFFER_SIZE - 1);
				Serial.println(F(" characters!"));
				input_buf_ptr = 0;
			} else {
				input_buf[input_buf_ptr] = '\0';
				input_buf_ptr = 0;
				
				// process the line	
				if (process_line(input_buf)) {
					Serial.print(F("> "));
				}
			}
		} else {
			if (input_buf_ptr < INPUT_BUFFER_SIZE) {
				if (c != '\r') { // Ignore carriage return
					input_buf[input_buf_ptr++] = c;
				}
			}
		}
	} 
}

static void print_hex_buffer(const char* buf, int len) {
	for (int i = 0; i < len; i++) {
		if (buf[i] == '\0') break;
		Serial.print('[');
		Serial.print((byte)buf[i], HEX);
		Serial.print(']');
	}
	Serial.println();
}

static size_t begins_with(const char* line, const char* prefix_P) {
	size_t len = strlen_P(prefix_P);
	if (strncmp_P(line, prefix_P, len) == 0 && (line[len] == ' ' || line[len] == '\0')) {
		return (line[len] == ' ') ? len + 1 : len;
	}
	return  0;
}

static bool process_line(const char* line) {
	size_t n;
	if (begins_with(line, PSTR("help")) > 0) {
		Serial.println(F("Commands:"));
		Serial.println(F("  help                    Show this menu"));
		Serial.println(F("  examples <|detailed>    Show examples for the commands"));
		Serial.println(F("  led <on|off>            Turn built-in LED on or off"));
		Serial.println(F("  at <AT+cmd>             Send command to GSM module"));
		Serial.println();
	} else if ((n=begins_with(line, PSTR("examples"))) > 0) {
		bool detailed = false;
		if (begins_with(line+n, PSTR("detailed")) > 0) {
			detailed = true;
		}

		Serial.println(F("Examples:"));
		Serial.println(F("  at AT+CSQ        Check signal strength <rssi>,<ber>"));
		if (detailed) {
			Serial.println(F("                   Received Signal Strength Indication (rssi) and Bit Error Rate (ber)"));
			Serial.println(F("                   rssi 0-9: marginal signal strength (-113 to -95 dBm)"));
			Serial.println(F("                   rssi 10-14: ok signal strength (-93 to -85 dBm), minimum for reliable SMS"));
			Serial.println(F("                   rssi 15-19: good signal strength (-83 to -75 dBm), stable LTE connection"));
			Serial.println(F("                   rssi 20-31: excellent signal strength (-73 to -51 dBm), strongest possible signal"));
			Serial.println(F("                   rssi/ber 99: No signal or not detected / not measured or signal too weak to measure"));
		}
		Serial.println(F("  at AT+CPIN?      Check SIM status"));
		Serial.println(F("  at AT+CREG?      Check network registration status <n>,<stat>"));
		if (detailed) {                      
			Serial.println(F("                   stat 0: Not registered - The module is not searching for a new operator"));
			Serial.println(F("                   stat 1: Registered (home) - You are on your carrier's native network."));
			Serial.println(F("                   stat 2: Searching - The module is hunting for a base station"));
			Serial.println(F("                   stat 3: Registration denied - The tower sees you, but won't let you in. Often SIM or IMEI issue."));
			Serial.println(F("                   stat 4: Unknown - Rare, usually indicates a hardware or antenna fault"));
			Serial.println(F("                   stat 5: Registered (roaming) - You are on partner network. SMS will still work, but may cost more"));
		}                                    
		Serial.println(F("  at ATI           Get hardware information"));
		Serial.println(F("  at ATE<0|1>      Turn echo off or on"));
		Serial.println(F("  at AT            Check communication link"));
		Serial.println(F("  at AT+CMGF=1     Set mode to text mode"));
		Serial.println(F("  at AT+CMGS=\"+3721234567890\"       Send SMS, message contents not supported"));
		Serial.println();
	} else if ((n=begins_with(line, PSTR("led"))) > 0) {
		if (begins_with(line + n, PSTR("on")) > 0) {
			digitalWrite(LED_BUILTIN, HIGH);
			Serial.println(F("Builtin LED turned on"));
		} else if (begins_with(line + n, PSTR("off")) > 0) {
			digitalWrite(LED_BUILTIN, LOW);
			Serial.println(F("Builtin LED turned off"));
		} else {
			Serial.print(F("Unknown operand '"));
			Serial.print(line + n);
			Serial.println('\'');
		}
	} else if ((n=begins_with(line, PSTR("at"))) > 0) {
		if (sms.sendCMD(line+n)) {
			sms.setWriteToSerial(true);
			return false;
		} else {
			Serial.println(F("[Modem busy]"));
		}
	} else if ((n=begins_with(line, PSTR("get"))) > 0) {
		int m;
		if ((m=begins_with(line+n, PSTR("number"))) > 0) {
			Serial.print(F("Currently defined phone number for SMS is "));
			Serial.print(Config::getPhoneNumber());
			Serial.println();
		}
	} else if ((n=begins_with(line, PSTR("set"))) > 0) {
		int m;
		if ((m=begins_with(line+n, PSTR("number"))) > 0) {
			const char* number = line + n + m;
			Config::setPhoneNumber(number);
			Serial.print(F("Set phone number for SMS alerts to "));
			Serial.println(number);
		}
	} else if ((n=begins_with(line, PSTR("sms"))) > 0) {
		if (sms.sendSMS(Message(Config::getPhoneNumber(), F("This is test SMS sending")))) {
			Serial.println(F("Sent test SMS message!"));
		} else {
			Serial.println(F("[Modem busy]"));
		}
	} else {
		Serial.print(F("Unkown command '"));
		Serial.print(line);
		Serial.println(F("'. Write 'help' to get help message."));
	}
	return true;
}