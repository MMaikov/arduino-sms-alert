#include "Application.hpp"

#include <avr/boot.h>
#include <avr/wdt.h>

Application::Application()
    : m_SMS(SMS_MODULE_RX, SMS_MODULE_TX)
{
}

void Application::Init() 
{
    wdt_enable(WDTO_8S);
    
    pinMode(LED_BUILTIN, OUTPUT);
    
	m_SMS.begin(SMS_MODULE_BAUD_RATE);
    
	Serial.begin(TERMINAL_BAUD_RATE);
	Serial.println(F("\r\nArduino SMS Alert system"));
	Serial.print(F("> "));
}

void Application::Update()
{
    wdt_reset();

	m_SMS.update();


	HandleSerialCommands();
}

void Application::HandleSerialCommands() 
{
	while (Serial.available() > 0) {
		char c = Serial.read();
		if (c == '\b' || c == 0x7f) { // Backspace
			if (m_InputBufferPtr > 0) {  
		        Serial.print(F("\b \b")); // Only echo when there is actually something to remove
				m_InputBufferPtr--;
			}
		} else if (c == '\n') {
            Serial.write(c); // Echo
			
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
                Serial.write(c); // Echo
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
		Serial.println(F("  info                    Show microcontroller information"));
		Serial.println(F("  sms                     Test SMS sending"));
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
	} else if (BeginsWith(line, PSTR("info")) > 0) {
        PrintSystemInfo();
    } else {
		Serial.print(F("Unkown command '"));
		Serial.print(line);
		Serial.println(F("'. Write 'help' to get help message."));
	}
	return true;
}

static float readInternalTemp() {
  // 1. Save the current ADC multiplexer settings to restore later
  uint8_t oldADMUX = ADMUX;

  // 2. Configure ADC: Internal 1.1V ref, select Channel 8 (Temp Sensor)
  // REFS1 and REFS0 set to 1 for 1.1V Internal Reference
  // MUX3 set to 1 for Temp Sensor
  ADMUX = (1 << REFS1) | (1 << REFS0) | (1 << MUX3);
  
  // Give the reference voltage a moment to settle
  delay(10);

  // 3. Start conversion
  ADCSRA |= (1 << ADSC);

  // 4. Wait for completion
  while (bit_is_set(ADCSRA, ADSC));

  // 5. Read the raw ADC value
  uint8_t low  = ADCL;
  uint8_t high = ADCH;
  int rawValue = (high << 8) | low;

  // 6. Restore the original ADMUX settings (restores analogRead compatibility)
  ADMUX = oldADMUX;
  
  // 7. Calculate Celsius (Adjust 324.31 for calibration)
  return (rawValue - 324.31) / 1.22;
}

static long readVcc() {
  long result;
  
  // 1. Save ADMUX to restore it later
  uint8_t oldADMUX = ADMUX;

  // 2. Set the reference to Vcc and the input to the internal 1.1V bandgap
  // REFS1:0 = 01 (Vcc as Ref), MUX3:0 = 1110 (1.1V Bandgap as Input)
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);

  delay(2); // Wait for Vref to settle

  // 3. Start conversion
  ADCSRA |= _BV(ADSC); 
  while (bit_is_set(ADCSRA, ADSC));

  // 4. Calculate Vcc in millivolts
  // 1125300 = 1.1 * 1023 * 1000
  result = ADCL;
  result |= ADCH << 8;
  result = 1125300L / result; 

  // 5. Restore ADMUX
  ADMUX = oldADMUX;
  
  return result; 
}

void Application::PrintSystemInfo() {
    FuseReport fuseReport = Config::getFuseReport();

    const auto& printBoolean = [](bool val){
        if (val) {
            Serial.println(F("yes"));
        } else {
            Serial.println(F("no"));
        }
    };

    Serial.print(F("Clock divide by 8: "));
    printBoolean(fuseReport.clockDivide8);

    Serial.print(F("Output clock on PB0 (pin 8): "));
    printBoolean(fuseReport.outputClockOnPortB0);

    Serial.print(F("Clock source: "));
    switch (fuseReport.clockSource)
    {
    case ClockSource::External_Clock:
        Serial.println(F("External clock"));
        break;
    case ClockSource::Internal_8MHz:
        Serial.println(F("Internal 8 MHz"));
        break;
    case ClockSource::Internal_128kHz:
        Serial.println(F("Internal 128 KHz"));
        break;
    case ClockSource::Ext_Crystal_LowPower:
        Serial.println(F("External Crystal low-power"));
        break;
    case ClockSource::Ext_Crystal_FullSwing:
        Serial.println(F("External Crystal fullswing"));
        break;
    default:
        Serial.println(F("Unknown"));
        break;
    }

    Serial.print(F("Startup time: "));
    switch (fuseReport.startupTime)
    {
    case StartupTime::Cycles_6_0ms:
        Serial.println(F("6 clock cycles, 0 ms delay"));
        break;
    case StartupTime::Cycles_6_4ms:
        Serial.println(F("6 clock cycles, 4 ms delay"));
        break;
    case StartupTime::Cycles_6_65ms:
        Serial.println(F("6 clock cycles, 65 ms delay"));
        break;
    case StartupTime::Cycles_258_4ms:
        Serial.println(F("258 clock cycles, 4 ms delay"));
        break;
    case StartupTime::Cycles_258_65ms:
        Serial.println(F("258 clock cycles, 65 ms delay"));
        break;
    case StartupTime::Cycles_1K_0ms:
        Serial.println(F("1000 clock cycles, 0 ms delay"));
        break;
    case StartupTime::Cycles_1K_4ms:
        Serial.println(F("1000 clock cycles, 4 ms delay"));
        break;
    case StartupTime::Cycles_1K_65ms:
        Serial.println(F("1000 clock cycles, 65 ms delay"));
        break;
    case StartupTime::Cycles_16K_0ms:
        Serial.println(F("16000 clock cycles, 0 ms delay"));
        break;
    case StartupTime::Cycles_16K_4ms:
        Serial.println(F("16000 clock cycles, 4 ms delay"));
        break;
    case StartupTime::Cycles_16K_65ms:
        Serial.println(F("16000 clock cycles, 65 ms delay"));
        break;
    case StartupTime::Cycles_1K_32ms:
        Serial.println(F("1000 clock cycles, 32 ms delay"));
        break;
    case StartupTime::Cycles_32K_65ms:
        Serial.println(F("32000 clock cycles, 65 ms delay"));
        break;
    case StartupTime::Reserved:
        Serial.println(F("Reserved"));
        break;
    default:
        Serial.println(F("Invalid"));
        break;
    }

    Serial.print(F("External reset disabled: "));
    printBoolean(fuseReport.externalResetDisabled);

    Serial.print(F("Debug wire enabled: "));
    printBoolean(fuseReport.debugWireEnabled);

    Serial.print(F("SPI enabled: "));
    printBoolean(fuseReport.spiEnabled);

    Serial.print(F("Watchdog always on: "));
    printBoolean(fuseReport.watchdogAlwaysOn);

    Serial.print(F("Preserve EEPROM: "));
    printBoolean(fuseReport.preserveEeprom);

    Serial.print(F("Boot size: "));
    switch (fuseReport.bootSize)
    {
    case BootSize::Words_256:
        Serial.println(F("256 words (512 bytes)"));
        break;
    case BootSize::Words_512:
        Serial.println(F("512 words (1024 bytes)"));
        break;
    case BootSize::Words_1024:
        Serial.println(F("1024 words (2048 bytes)"));
        break;
    case BootSize::Words_2048:
        Serial.println(F("2048 words (4096 bytes)"));
        break;
    default:
        Serial.println(F("Unknown"));
        break;
    }

    Serial.print(F("Boot to loader: "));
    printBoolean(fuseReport.bootToLoader);

    Serial.print(F("BOD: "));
    switch (fuseReport.bod)
    {
    case BODLevel::Disabled:
        Serial.println(F("Disabled"));
        break;
    case BODLevel::V1_8:
        Serial.println(F("1.8V"));
        break;
    case BODLevel::V2_7:
        Serial.println(F("2.7V"));
        break;
    case BODLevel::V4_3:
        Serial.println(F("4.3V"));
        break;
    default:
        Serial.println(F("Unknown"));
        break;
    }

    Serial.print(F("Chip lock: "));
    switch (fuseReport.chipLock)
    {
    case MemoryLock::None:
        Serial.println(F("None"));
        break;
    case MemoryLock::NoWrite:
        Serial.println(F("No write"));
        break;
    case MemoryLock::NoReadWrite:
        Serial.println(F("No Read/Write"));
        break;    
    default:
        Serial.println(F("Unknown"));
        break;
    }

    const auto& printSectionLock = [](SectionLock sectionLock) {
        switch (sectionLock)
        {
        case SectionLock::None:
            Serial.println(F("None"));
            break;
        case SectionLock::NoWrite:
            Serial.println(F("No Write"));
            break;
        case SectionLock::NoReadWrite:
            Serial.println(F("No Read/Write"));
            break;
        case SectionLock::NoRead:
            Serial.println(F("No Read"));
            break;
        default:
            Serial.println(F("Unknown"));
            break;
        }
    };

    Serial.print(F("App section: "));
    printSectionLock(fuseReport.appSection);

    Serial.print(F("Boot section: "));
    printSectionLock(fuseReport.bootSection);

    Serial.print(F("Fuses: 0x"));
    Serial.print(fuseReport.lowFuse, HEX);
    Serial.print(F(" (low) 0x"));
    Serial.print(fuseReport.highFuse, HEX);
    Serial.print(F(" (high) 0x"));
    Serial.print(fuseReport.extFuse, HEX);
    Serial.print(F(" (ext) 0x"));
    Serial.print(fuseReport.lockBits, HEX);
    Serial.println(F(" (lock)"));

    Serial.print(F("Internal temperature: "));
    Serial.println(readInternalTemp(), 1);

    Serial.print(F("Supply voltage: "));
    Serial.print(readVcc());
    Serial.println(F(" mV"));

    Serial.print(F("OSCCAL: "));
    Serial.println(OSCCAL);

    Serial.print("Device Signature: ");
    Serial.print(boot_signature_byte_get(0x00), HEX);
    Serial.print(" ");
    Serial.print(boot_signature_byte_get(0x02), HEX);
    Serial.print(" ");
    Serial.println(boot_signature_byte_get(0x04), HEX);
}
