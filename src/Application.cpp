#include "Application.hpp"

Application::Application()
    : m_SMS(SMS_MODULE_RX, SMS_MODULE_TX)
{
}

void Application::Init() 
{
	pinMode(LED_BUILTIN, OUTPUT);

	m_SMS.begin(SMS_MODULE_BAUD_RATE);
    
	Serial.begin(TERMINAL_BAUD_RATE);
	Serial.println(F("\r\nArduino SMS Alert system"));
	Serial.print(F("> "));
}

void Application::Update()
{
	m_SMS.update();

	HandleSerialCommands();
}

void Application::HandleSerialCommands() 
{
	while (Serial.available() > 0) {
		char c = Serial.read();
		Serial.write(c); // Echo
		if (c == '\b' || c == 0x7f) { // Backspace
			if (m_InputBufferPtr > 0) {
				m_InputBufferPtr--;
			}
		} else if (c == '\n') {
			if (m_InputBufferPtr >= INPUT_BUFFER_SIZE) {
				Serial.print(F("input discarded, max only "));
				Serial.print(INPUT_BUFFER_SIZE - 1);
				Serial.println(F(" characters!"));
				m_InputBufferPtr = 0;
			} else {
				m_InputBuffer[m_InputBufferPtr] = '\0';
				m_InputBufferPtr = 0;
				
				// process the line	
				if (ProcessLine(m_InputBuffer)) {
					Serial.print(F("> "));
				}
			}
		} else {
			if (m_InputBufferPtr < INPUT_BUFFER_SIZE) {
				if (c != '\r') { // Ignore carriage return
					m_InputBuffer[m_InputBufferPtr++] = c;
				}
			}
		}
	} 
}

size_t Application::BeginsWith(const char* line, const char* prefix_P) 
{
	size_t len = strlen_P(prefix_P);
	if (strncmp_P(line, prefix_P, len) == 0 && (line[len] == ' ' || line[len] == '\0')) {
		return (line[len] == ' ') ? len + 1 : len;
	}
	return  0;
}

bool Application::ProcessLine(const char* line) 
{
	size_t n;
	if (BeginsWith(line, PSTR("help")) > 0) {
		Serial.println(F("Commands:"));
		Serial.println(F("  help                    Show this menu"));
		Serial.println(F("  examples <|detailed>    Show examples for the commands"));
		Serial.println(F("  led <on|off>            Turn built-in LED on or off"));
		Serial.println(F("  at <AT+cmd>             Send command to GSM module"));
		Serial.println(F("  get <number>            Get parameter data"));
		Serial.println(F("  set <number>            Set parameter data"));
		Serial.println();
	} else if ((n=BeginsWith(line, PSTR("examples"))) > 0) {
		bool detailed = false;
		if (BeginsWith(line+n, PSTR("detailed")) > 0) {
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
	} else if ((n=BeginsWith(line, PSTR("led"))) > 0) {
		if (BeginsWith(line + n, PSTR("on")) > 0) {
			digitalWrite(LED_BUILTIN, HIGH);
			Serial.println(F("Builtin LED turned on"));
		} else if (BeginsWith(line + n, PSTR("off")) > 0) {
			digitalWrite(LED_BUILTIN, LOW);
			Serial.println(F("Builtin LED turned off"));
		} else {
			Serial.print(F("Unknown operand '"));
			Serial.print(line + n);
			Serial.println('\'');
		}
	} else if ((n=BeginsWith(line, PSTR("at"))) > 0) {
		if (m_SMS.sendCMD(line+n)) {
			m_SMS.setWriteToSerial(true);
			return false;
		} else {
			Serial.println(F("[Modem busy]"));
		}
	} else if ((n=BeginsWith(line, PSTR("get"))) > 0) {
		size_t m;
		if ((m=BeginsWith(line+n, PSTR("number"))) > 0) {
			Serial.print(F("Currently defined phone number for SMS is "));
			Serial.print(Config::getPhoneNumber());
			Serial.println();
		}
	} else if ((n=BeginsWith(line, PSTR("set"))) > 0) {
		size_t m;
		if ((m=BeginsWith(line+n, PSTR("number"))) > 0) {
			const char* number = line + n + m;
			Config::setPhoneNumber(number);
			Serial.print(F("Set phone number for SMS alerts to "));
			Serial.println(number);
		}
	} else if ((n=BeginsWith(line, PSTR("sms"))) > 0) {
		if (m_SMS.sendSMS(Message(Config::getPhoneNumber(), F("This is test SMS sending")))) {
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