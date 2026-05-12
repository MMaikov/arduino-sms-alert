#include <Arduino.h>
#include <SoftwareSerial.h>

#define SMS_MODULE_RX 7
#define SMS_MODULE_TX 8
#define SMS_MODULE_BAUD_RATE 9600

// 40 characters + null terminator
#define INPUT_BUFFER_SIZE 41

static uint8_t input_buf_ptr = 0;
static char input_buf[INPUT_BUFFER_SIZE];

static SoftwareSerial smsSerial(SMS_MODULE_RX, SMS_MODULE_TX);

static void handle_serial_commands();
static void process_line(const char* line);

void setup() {
	pinMode(LED_BUILTIN, OUTPUT);

	Serial.begin(9600);
	Serial.println(F("Arduino SMS Alert system"));
	Serial.print(F("> "));

	smsSerial.begin(SMS_MODULE_BAUD_RATE);
}

void loop() {
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
				process_line(input_buf);
				Serial.print(F("> "));
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

static void process_line(const char* line) {
	size_t n;
	if (begins_with(line, PSTR("help")) > 0) {
		Serial.println(F("Commands:"));
		Serial.println(F("  help                    Show this menu"));
		Serial.println(F("  examples <|detailed>    Show examples for the commands"));
		Serial.println(F("  led <on|off>            Turn built-in LED on or off"));
		Serial.println(F("  sms <AT+cmd>            Send command to GSM module"));
		Serial.println();
	} else if ((n=begins_with(line, PSTR("examples"))) > 0) {
		bool detailed = false;
		if (begins_with(line+n, PSTR("detailed")) > 0) {
			detailed = true;
		}

		Serial.println(F("Examples:"));
		Serial.println(F("  sms AT+CSQ       Check signal strength <rssi>,<ber>"));
		if (detailed) {
			Serial.println(F("                   Received Signal Strength Indication (rssi) and Bit Error Rate (ber)"));
			Serial.println(F("                   rssi 0-9: marginal signal strength (-113 to -95 dBm)"));
			Serial.println(F("                   rssi 10-14: ok signal strength (-93 to -85 dBm), minimum for reliable SMS"));
			Serial.println(F("                   rssi 15-19: good signal strength (-83 to -75 dBm), stable LTE connection"));
			Serial.println(F("                   rssi 20-31: excellent signal strength (-73 to -51 dBm), strongest possible signal"));
			Serial.println(F("                   rssi/ber 99: No signal or not detected / not measured or signal too weak to measure"));
		}
		Serial.println(F("  sms AT+CPIN?     Check SIM status"));
		Serial.println(F("  sms AT+CREG?     Check network registration status <n>,<stat>"));
		if (detailed) {                      
			Serial.println(F("                   stat 0: Not registered - The module is not searching for a new operator"));
			Serial.println(F("                   stat 1: Registered (home) - You are on your carrier's native network."));
			Serial.println(F("                   stat 2: Searching - The module is hunting for a base station"));
			Serial.println(F("                   stat 3: Registration denied - The tower sees you, but won't let you in. Often SIM or IMEI issue."));
			Serial.println(F("                   stat 4: Unknown - Rare, usually indicates a hardware or antenna fault"));
			Serial.println(F("                   stat 5: Registered (roaming) - You are on partner network. SMS will still work, but may cost more"));
		}                                    
		Serial.println(F("  sms ATI          Get hardware information"));
		Serial.println(F("  sms ATE<0|1>     Turn echo off or on"));
		Serial.println(F("  sms AT           Check communication link"));
		Serial.println(F("  sms AT+CMGF=1    Set mode to text mode"));
		Serial.println(F("  sms AT+CMGS=\"+372 1234567890\"     Send SMS, message contents not supported"));
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
	} else if ((n=begins_with(line, PSTR("sms"))) > 0) {
		smsSerial.println(line+n);
		
		const unsigned long startTime = millis();
		while (millis() - startTime < 2000) {
			if (smsSerial.available() > 0) {
				const char currentChar = smsSerial.read();
				Serial.write(currentChar);

				if (currentChar == '>') {
					smsSerial.write(0x1A); // To finish
				}
			}
		}
		Serial.println();
	} else {
		Serial.print(F("Unkown command '"));
		Serial.print(line);
		Serial.println(F("'. Write 'help' to get help message."));
	}
}