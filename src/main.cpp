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
		Serial.println(F("Help message (TODO):"));
		Serial.println(F("<help> for this help message"));
		Serial.println(F("<led on|off> to turn on or off the builtin LED"));
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
		Serial.println(F("'! Write 'help' to get help message."));
	}
}