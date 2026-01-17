#include <Arduino.h>
#include "LoRaComm.h"
#include "MessageProtocol.h"
#include "DummySensors.h"
#include "board_config.h"

// ===== Global Objects =====
LoRaComm loraComm;
MessageProtocol protocol;
DummySensors sensors;

// ===== Statistics =====
struct Statistics {
    unsigned long messagesReceived;
    unsigned long messagesFailed;
    long totalRSSI;
    unsigned long rssiCount;
    unsigned long startTime;
};

Statistics stats = {0, 0, 0, 0, 0};

// ===== Buffers =====
uint8_t rxBuffer[MSG_MAX_PACKET_SIZE];
Message lastMessage;

// ===== LED Blink Function =====
void blinkLED() {
    digitalWrite(LED_PIN, HIGH);
    delay(50);  // Brief 50ms flash
    digitalWrite(LED_PIN, LOW);
}

void setup() {
    // Initialize Serial
    Serial.begin(SERIAL_BAUD);
    delay(1500);

    // Initialize LED pin
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // Print banner
    Serial.println(F("\n\n"));
    Serial.println(F("===================================="));
    Serial.println(F("  LoRa Ra-02 RECEIVER"));
    Serial.println(F("  Receives Sensor Data"));
    Serial.println(F("===================================="));
    Serial.print(F("Board: "));
    Serial.println(BOARD_NAME);
    Serial.print(F("LED Pin: "));
    Serial.println(LED_PIN);
    Serial.println();

    // Initialize LoRa
    Serial.println(F("Initializing LoRa module..."));
    if (!loraComm.begin()) {
        Serial.println(F("\nFATAL: LoRa initialization failed!"));
        Serial.println(F("System halted. Check wiring and reset board."));
        while (1) {
            delay(1000);
        }
    }

    // Initialize sensors (for getting sensor names)
    sensors.begin();

    stats.startTime = millis();

    Serial.println();
    Serial.println(F("===================================="));
    Serial.println(F("  System Ready - Listening"));
    Serial.println(F("===================================="));
    Serial.println(F("Waiting for sensor data..."));
    Serial.println(F("===================================="));
    Serial.println();
}

void loop() {
    // Check for incoming LoRa packets
    int packetSize = loraComm.receivePacket(rxBuffer, sizeof(rxBuffer));

    if (packetSize > 0) {
        stats.messagesReceived++;
        stats.totalRSSI += loraComm.getRSSI();
        stats.rssiCount++;

        // Blink LED on packet received
        blinkLED();

        // Debug: Print raw packet info
        Serial.print(F("[DEBUG] Received "));
        Serial.print(packetSize);
        Serial.print(F(" bytes, RSSI: "));
        Serial.print(loraComm.getRSSI());
        Serial.print(F(" | Raw: "));
        for (int i = 0; i < min(packetSize, 20); i++) {
            if (rxBuffer[i] < 0x10) Serial.print('0');
            Serial.print(rxBuffer[i], HEX);
            Serial.print(' ');
        }
        if (packetSize > 20) Serial.print(F("..."));
        Serial.println();

        // Decode message
        if (protocol.decode(rxBuffer, packetSize, lastMessage)) {
            lastMessage.rssi = loraComm.getRSSI();
            lastMessage.snr = loraComm.getSNR();

            // Process based on message type
            if (lastMessage.type == MSG_SENSOR_RESPONSE) {
                // Parse sensor response - try with device name first, then fallback to legacy
                SensorData data;
                bool parsed = protocol.parseSensorResponseWithDevice(lastMessage.payload, lastMessage.payloadLength, data);
                if (!parsed) {
                    // Fallback to legacy parsing (no device name)
                    parsed = protocol.parseSensorResponse(lastMessage.payload, lastMessage.payloadLength, data);
                }

                // Debug: Show parsing result
                Serial.print(F("[DEBUG] Parse with device: "));
                Serial.print(parsed ? F("OK") : F("FAIL"));
                if (parsed) {
                    Serial.print(F(", Device='"));
                    Serial.print(data.deviceName);
                    Serial.print(F("', Sensor="));
                    Serial.println(data.sensorId);
                } else {
                    Serial.println();
                }

                if (parsed) {
                    // Display sensor data
                    unsigned long uptime = (millis() - stats.startTime) / 1000;

                    Serial.print(F("["));
                    Serial.print(uptime);
                    Serial.print(F("s] "));

                    // Display device name if available
                    if (data.deviceName[0] != '\0') {
                        Serial.print(F("["));
                        Serial.print(data.deviceName);
                        Serial.print(F("] "));
                    }

                    Serial.print(sensors.getSensorName(data.sensorId));
                    Serial.print(F(": "));
                    Serial.print(data.value, 2);
                    Serial.print(F(" "));
                    Serial.print(data.unit);

                    Serial.print(F(" | RSSI: "));
                    Serial.print(lastMessage.rssi);
                    Serial.print(F(" dBm | SNR: "));
                    Serial.print(lastMessage.snr, 1);
                    Serial.print(F(" dB | ID: "));
                    Serial.println(lastMessage.messageId);
                } else {
                    Serial.println(F("[ERROR] Failed to parse sensor data"));
                    stats.messagesFailed++;
                }
            } else if (lastMessage.type == MSG_TEXT) {
                // Display text message
                char textBuffer[MSG_MAX_PAYLOAD + 1];
                memcpy(textBuffer, lastMessage.payload, lastMessage.payloadLength);
                textBuffer[lastMessage.payloadLength] = '\0';

                unsigned long uptime = (millis() - stats.startTime) / 1000;

                Serial.print(F("["));
                Serial.print(uptime);
                Serial.print(F("s] TEXT: \""));
                Serial.print(textBuffer);
                Serial.print(F("\" | RSSI: "));
                Serial.print(lastMessage.rssi);
                Serial.print(F(" dBm | SNR: "));
                Serial.print(lastMessage.snr, 1);
                Serial.println(F(" dB"));
            } else {
                // Unknown message type
                Serial.print(F("[RX] Unknown message type: 0x"));
                Serial.println(lastMessage.type, HEX);
            }
        } else {
            Serial.println(F("[ERROR] Failed to decode packet (checksum error)"));
            stats.messagesFailed++;
        }

        // Print statistics every 20 messages
        if (stats.messagesReceived % 20 == 0) {
            Serial.println();
            Serial.println(F("--- Statistics ---"));
            Serial.print(F("Received: "));
            Serial.println(stats.messagesReceived);
            Serial.print(F("Failed: "));
            Serial.println(stats.messagesFailed);
            if (stats.rssiCount > 0) {
                Serial.print(F("Avg RSSI: "));
                Serial.print(stats.totalRSSI / stats.rssiCount);
                Serial.println(F(" dBm"));
            }
            Serial.print(F("Uptime: "));
            Serial.print((millis() - stats.startTime) / 1000);
            Serial.println(F(" seconds"));
            Serial.println(F("------------------"));
            Serial.println();
        }
    }

    delay(10);
}
