#include <Arduino.h>
#include "LoRaComm.h"
#include "MessageProtocol.h"
#include "DummySensors.h"
#include "board_config.h"

// ===== Global Objects =====
LoRaComm loraComm;
MessageProtocol protocol;
DummySensors sensors;

// ===== Configuration =====
const unsigned long SEND_INTERVAL = 5000;  // Send every 5 seconds
unsigned long lastSendTime = 0;
uint8_t currentSensor = SENSOR_TEMPERATURE;  // Start with temperature

// ===== Buffer =====
uint8_t txBuffer[MSG_MAX_PACKET_SIZE];

void setup() {
    // Initialize Serial
    Serial.begin(SERIAL_BAUD);
    delay(1500);

    // Print banner
    Serial.println(F("\n\n"));
    Serial.println(F("===================================="));
    Serial.println(F("  LoRa Ra-02 SENDER"));
    Serial.println(F("  Transmits Dummy Sensor Data"));
    Serial.println(F("===================================="));
    Serial.print(F("Board: "));
    Serial.println(BOARD_NAME);
    Serial.print(F("Device: "));
    Serial.println(DEVICE_NAME);
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

    // Initialize sensors
    sensors.begin();
    Serial.println(F("Dummy sensors initialized"));

    Serial.println();
    Serial.println(F("===================================="));
    Serial.println(F("  System Ready - Transmitting"));
    Serial.println(F("===================================="));
    Serial.println(F("Sending sensor data every 5 seconds..."));
    Serial.println(F("Rotating: Temp → Humid → Bat → Pressure"));
    Serial.println(F("===================================="));
    Serial.println();
}

void loop() {
    unsigned long currentTime = millis();

    // Send sensor data at interval
    if (currentTime - lastSendTime >= SEND_INTERVAL) {
        lastSendTime = currentTime;

        // Read sensor
        float value;
        const char* unit;
        const char* name;
        uint8_t sensorToSend = currentSensor;  // Save current sensor ID before updating

        switch (currentSensor) {
            case SENSOR_TEMPERATURE:
                value = sensors.readTemperature();
                unit = sensors.getSensorUnit(SENSOR_TEMPERATURE);
                name = sensors.getSensorName(SENSOR_TEMPERATURE);
                currentSensor = SENSOR_HUMIDITY;  // Next sensor
                break;
            case SENSOR_HUMIDITY:
                value = sensors.readHumidity();
                unit = sensors.getSensorUnit(SENSOR_HUMIDITY);
                name = sensors.getSensorName(SENSOR_HUMIDITY);
                currentSensor = SENSOR_BATTERY;
                break;
            case SENSOR_BATTERY:
                value = sensors.readBatteryVoltage();
                unit = sensors.getSensorUnit(SENSOR_BATTERY);
                name = sensors.getSensorName(SENSOR_BATTERY);
                currentSensor = SENSOR_PRESSURE;
                break;
            case SENSOR_PRESSURE:
                value = sensors.readPressure();
                unit = sensors.getSensorUnit(SENSOR_PRESSURE);
                name = sensors.getSensorName(SENSOR_PRESSURE);
                currentSensor = SENSOR_TEMPERATURE;  // Loop back
                break;
            default:
                currentSensor = SENSOR_TEMPERATURE;
                return;
        }

        // Encode sensor response with device name (use saved sensor ID)
        size_t len = protocol.encodeSensorResponseWithDevice(DEVICE_NAME, sensorToSend, value, unit, txBuffer);

        if (len > 0 && loraComm.sendPacket(txBuffer, len)) {
            Serial.print(F("[TX] "));
            Serial.print(name);
            Serial.print(F(": "));
            Serial.print(value, 2);
            Serial.print(F(" "));
            Serial.print(unit);
            Serial.print(F(" ("));
            Serial.print(len);
            Serial.println(F(" bytes)"));
        } else {
            Serial.println(F("[ERROR] Failed to send packet"));
        }
    }

    delay(10);
}
