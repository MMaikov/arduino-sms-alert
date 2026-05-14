#include "SMS.hpp"

SMS::SMS(uint8_t rx, uint8_t tx)
		: m_SmsSerial(rx, tx), m_Command(nullptr), m_Status(ModemStatus::Ready), m_LastTime(millis())
{
}

void SMS::begin(long speed) {
    pinMode(SMS_MODULE_PWRKEY, OUTPUT);
    
    digitalWrite(SMS_MODULE_PWRKEY, HIGH);
    delay(1000);
    digitalWrite(SMS_MODULE_PWRKEY, LOW);

    m_SmsSerial.begin(speed);
}

bool SMS::sendSMS(const Message& message, bool priority) {
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

bool SMS::sendCMD(const char* command) {
    if (m_Status == ModemStatus::Ready) {
        m_Command = command;
        m_Status = ModemStatus::SendCMD;
        m_LastTime = millis();
        return true;
    }
    return false;
}

void SMS::setWriteToSerial(bool status) {
    m_WriteToSerial = status;
}

void SMS::update() {
    
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
            Serial.print(F("\r\n[Timeout]> "));
        }
    }
}