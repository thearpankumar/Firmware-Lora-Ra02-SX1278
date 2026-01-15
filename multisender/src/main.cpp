#include <Arduino.h>
#include "DualLoRaComm.h"
#include "MessageProtocol.h"
#include "DummySensors.h"
#include "board_config.h"

// ===== Global Objects =====
DualLoRaComm dualLora;
MessageProtocol protocol;
DummySensors sensors;

// ===== Configuration =====
const unsigned long SEND_INTERVAL = 5000;  // Send every 5 seconds
unsigned long lastSendTime = 0;

// ===== Module/Sensor State =====
// Alternate between modules and rotate through sensors
uint8_t currentModule = MODULE_1;  // Start with module 1
uint8_t currentSensor = SENSOR_TEMPERATURE;  // Start with temperature

// ===== Statistics =====
struct Statistics {
    unsigned long module1Sent;
    unsigned long module2Sent;
    unsigned long totalFailed;
    unsigned long startTime;
};

Statistics stats = {0, 0, 0, 0};

// ===== Buffer =====
uint8_t txBuffer[MSG_MAX_PACKET_SIZE];

void setup() {
    // Initialize Serial
    Serial.begin(SERIAL_BAUD);
    delay(1500);

    // Print banner
    Serial.println(F("\n\n"));
    Serial.println(F("=========================================="));
    Serial.println(F("  LoRa Ra-02 MULTI-SENDER"));
    Serial.println(F("  Dual Module Transmitter"));
    Serial.println(F("=========================================="));
    Serial.print(F("Board: "));
    Serial.println(BOARD_NAME);
    Serial.print(F("Module 1: "));
    Serial.println(LORA1_NAME);
    Serial.print(F("Module 2: "));
    Serial.println(LORA2_NAME);
    Serial.println();

    // Initialize Dual LoRa modules
    Serial.println(F("Initializing dual LoRa modules..."));
    if (!dualLora.begin()) {
        Serial.println(F("\nFATAL: Dual LoRa initialization failed!"));
        Serial.println(F("Check wiring for both modules and reset board."));
        while (1) {
            delay(1000);
        }
    }

    // Print configuration
    dualLora.printConfig();

    // Initialize sensors
    sensors.begin();
    Serial.println(F("Dummy sensors initialized"));

    stats.startTime = millis();

    Serial.println();
    Serial.println(F("=========================================="));
    Serial.println(F("  System Ready - Transmitting"));
    Serial.println(F("=========================================="));
    Serial.println(F("Sending sensor data every 5 seconds..."));
    Serial.println(F("Alternating: Module1 -> Module2 -> Module1..."));
    Serial.println(F("Rotating: Temp -> Humid -> Bat -> Pressure"));
    Serial.println(F("=========================================="));
    Serial.println();
}

void loop() {
    unsigned long currentTime = millis();

    // Send sensor data at interval
    if (currentTime - lastSendTime >= SEND_INTERVAL) {
        lastSendTime = currentTime;

        // Get current module's device name
        const char* deviceName = dualLora.getDeviceName(currentModule);

        // Read sensor data
        float value;
        const char* unit;
        const char* sensorName;

        switch (currentSensor) {
            case SENSOR_TEMPERATURE:
                value = sensors.readTemperature();
                unit = sensors.getSensorUnit(SENSOR_TEMPERATURE);
                sensorName = sensors.getSensorName(SENSOR_TEMPERATURE);
                break;
            case SENSOR_HUMIDITY:
                value = sensors.readHumidity();
                unit = sensors.getSensorUnit(SENSOR_HUMIDITY);
                sensorName = sensors.getSensorName(SENSOR_HUMIDITY);
                break;
            case SENSOR_BATTERY:
                value = sensors.readBatteryVoltage();
                unit = sensors.getSensorUnit(SENSOR_BATTERY);
                sensorName = sensors.getSensorName(SENSOR_BATTERY);
                break;
            case SENSOR_PRESSURE:
                value = sensors.readPressure();
                unit = sensors.getSensorUnit(SENSOR_PRESSURE);
                sensorName = sensors.getSensorName(SENSOR_PRESSURE);
                break;
            default:
                currentSensor = SENSOR_TEMPERATURE;
                return;
        }

        // Encode sensor response with device name
        size_t len = protocol.encodeSensorResponseWithDevice(deviceName, currentSensor, value, unit, txBuffer);

        // Send via current module
        if (len > 0 && dualLora.sendPacket(currentModule, txBuffer, len)) {
            // Update statistics
            if (currentModule == MODULE_1) {
                stats.module1Sent++;
            } else {
                stats.module2Sent++;
            }

            // Print transmission info
            Serial.print(F("[TX] ["));
            Serial.print(deviceName);
            Serial.print(F("] "));
            Serial.print(sensorName);
            Serial.print(F(": "));
            Serial.print(value, 2);
            Serial.print(F(" "));
            Serial.print(unit);
            Serial.print(F(" ("));
            Serial.print(len);
            Serial.println(F(" bytes)"));
        } else {
            stats.totalFailed++;
            Serial.print(F("[ERROR] Failed to send via "));
            Serial.println(deviceName);
        }

        // Rotate to next sensor
        switch (currentSensor) {
            case SENSOR_TEMPERATURE:
                currentSensor = SENSOR_HUMIDITY;
                break;
            case SENSOR_HUMIDITY:
                currentSensor = SENSOR_BATTERY;
                break;
            case SENSOR_BATTERY:
                currentSensor = SENSOR_PRESSURE;
                break;
            case SENSOR_PRESSURE:
                currentSensor = SENSOR_TEMPERATURE;
                break;
        }

        // Alternate to next module
        currentModule = (currentModule == MODULE_1) ? MODULE_2 : MODULE_1;

        // Print statistics every 20 messages
        unsigned long totalSent = stats.module1Sent + stats.module2Sent;
        if (totalSent > 0 && totalSent % 20 == 0) {
            Serial.println();
            Serial.println(F("--- Statistics ---"));
            Serial.print(F("Module 1 ("));
            Serial.print(dualLora.getDeviceName(MODULE_1));
            Serial.print(F("): "));
            Serial.println(stats.module1Sent);
            Serial.print(F("Module 2 ("));
            Serial.print(dualLora.getDeviceName(MODULE_2));
            Serial.print(F("): "));
            Serial.println(stats.module2Sent);
            Serial.print(F("Total sent: "));
            Serial.println(totalSent);
            Serial.print(F("Failed: "));
            Serial.println(stats.totalFailed);
            Serial.print(F("Uptime: "));
            Serial.print((millis() - stats.startTime) / 1000);
            Serial.println(F(" seconds"));
            Serial.println(F("------------------"));
            Serial.println();
        }
    }

    delay(10);
}
