#include "DualLoRaComm.h"

DualLoRaComm::DualLoRaComm() {
    // Initialize device names from build flags
    deviceNames[MODULE_1] = LORA1_NAME;
    deviceNames[MODULE_2] = LORA2_NAME;

    // Initialize pin configurations from build flags
    nssPins[MODULE_1] = LORA1_NSS;
    nssPins[MODULE_2] = LORA2_NSS;

    dio0Pins[MODULE_1] = LORA1_DIO0;
    dio0Pins[MODULE_2] = LORA2_DIO0;

    resetPins[MODULE_1] = LORA1_RESET;
    resetPins[MODULE_2] = LORA2_RESET;
}

bool DualLoRaComm::begin() {
    Serial.println(F("\n=== Dual LoRa Module Initialization ==="));

    // Initialize SPI bus (shared between both modules)
    Serial.println(F("Initializing shared SPI bus..."));
    SPI.begin();
    delay(50);

    // Deselect both modules first
    pinMode(nssPins[MODULE_1], OUTPUT);
    pinMode(nssPins[MODULE_2], OUTPUT);
    digitalWrite(nssPins[MODULE_1], HIGH);
    digitalWrite(nssPins[MODULE_2], HIGH);

    // Initialize Module 1
    Serial.println(F("\n--- Initializing Module 1 ---"));
    if (!initModule(lora1, nssPins[MODULE_1], dio0Pins[MODULE_1], resetPins[MODULE_1], deviceNames[MODULE_1])) {
        Serial.println(F("ERROR: Failed to initialize Module 1"));
        return false;
    }
    Serial.print(F("Module 1 ("));
    Serial.print(deviceNames[MODULE_1]);
    Serial.println(F(") initialized successfully"));

    // Small delay between module initializations
    delay(100);

    // Initialize Module 2
    Serial.println(F("\n--- Initializing Module 2 ---"));
    if (!initModule(lora2, nssPins[MODULE_2], dio0Pins[MODULE_2], resetPins[MODULE_2], deviceNames[MODULE_2])) {
        Serial.println(F("ERROR: Failed to initialize Module 2"));
        return false;
    }
    Serial.print(F("Module 2 ("));
    Serial.print(deviceNames[MODULE_2]);
    Serial.println(F(") initialized successfully"));

    Serial.println(F("\n=== Both modules initialized successfully ==="));
    return true;
}

bool DualLoRaComm::initModule(LoRaClass& lora, int nss, int dio0, int rst, const char* name) {
    Serial.print(F("  Name: "));
    Serial.println(name);
    Serial.print(F("  NSS: GPIO"));
    Serial.println(nss);
    Serial.print(F("  DIO0: GPIO"));
    Serial.println(dio0);
    Serial.print(F("  RESET: GPIO"));
    Serial.println(rst);

    // Set pins for this module
    lora.setPins(nss, rst, dio0);

    // Hardware reset
    pinMode(rst, OUTPUT);
    digitalWrite(rst, LOW);
    delay(10);
    digitalWrite(rst, HIGH);
    delay(100);

    // Initialize module
    if (!lora.begin(LORA_FREQUENCY)) {
        Serial.println(F("  ERROR: LoRa.begin() failed!"));
        Serial.println(F("  Check wiring and connections"));
        return false;
    }

    // Configure LoRa parameters
    configureModule(lora);

    return true;
}

void DualLoRaComm::configureModule(LoRaClass& lora) {
    lora.setSpreadingFactor(LORA_SPREADING_FACTOR);
    lora.setSignalBandwidth(LORA_SIGNAL_BANDWIDTH);
    lora.setCodingRate4(LORA_CODING_RATE);
    lora.setPreambleLength(LORA_PREAMBLE_LENGTH);
    lora.setSyncWord(LORA_SYNC_WORD);
    lora.setTxPower(LORA_TX_POWER);
    lora.enableCrc();
}

void DualLoRaComm::selectModule(uint8_t moduleIndex) {
    // Deselect all modules first
    digitalWrite(nssPins[MODULE_1], HIGH);
    digitalWrite(nssPins[MODULE_2], HIGH);

    // Select the requested module
    if (moduleIndex < NUM_LORA_MODULES) {
        digitalWrite(nssPins[moduleIndex], LOW);
    }
}

bool DualLoRaComm::sendPacket(uint8_t moduleIndex, const uint8_t* data, size_t length) {
    if (moduleIndex >= NUM_LORA_MODULES) {
        Serial.println(F("ERROR: Invalid module index"));
        return false;
    }

    if (length == 0 || length > 255) {
        Serial.println(F("ERROR: Invalid packet length"));
        return false;
    }

    // Get reference to correct module
    LoRaClass& lora = (moduleIndex == MODULE_1) ? lora1 : lora2;

    // Begin packet
    lora.beginPacket();

    // Write data
    lora.write(data, length);

    // End packet and transmit
    if (!lora.endPacket()) {
        Serial.print(F("ERROR: Packet transmission failed on module "));
        Serial.println(moduleIndex);
        return false;
    }

    return true;
}

const char* DualLoRaComm::getDeviceName(uint8_t moduleIndex) {
    if (moduleIndex >= NUM_LORA_MODULES) {
        return "Unknown";
    }
    return deviceNames[moduleIndex];
}

void DualLoRaComm::printConfig() {
    Serial.println(F("\n=== Dual LoRa Configuration ==="));
    Serial.print(F("Frequency: "));
    Serial.print(LORA_FREQUENCY / 1E6);
    Serial.println(F(" MHz"));

    Serial.print(F("Spreading Factor: SF"));
    Serial.println(LORA_SPREADING_FACTOR);

    Serial.print(F("Bandwidth: "));
    Serial.print(LORA_SIGNAL_BANDWIDTH / 1E3);
    Serial.println(F(" kHz"));

    Serial.print(F("Coding Rate: 4/"));
    Serial.println(LORA_CODING_RATE);

    Serial.print(F("TX Power: "));
    Serial.print(LORA_TX_POWER);
    Serial.println(F(" dBm"));

    Serial.print(F("Sync Word: 0x"));
    Serial.println(LORA_SYNC_WORD, HEX);

    Serial.println(F("\n--- Module 1 ---"));
    Serial.print(F("Name: "));
    Serial.println(deviceNames[MODULE_1]);
    Serial.print(F("NSS: GPIO"));
    Serial.print(nssPins[MODULE_1]);
    Serial.print(F(", DIO0: GPIO"));
    Serial.print(dio0Pins[MODULE_1]);
    Serial.print(F(", RST: GPIO"));
    Serial.println(resetPins[MODULE_1]);

    Serial.println(F("\n--- Module 2 ---"));
    Serial.print(F("Name: "));
    Serial.println(deviceNames[MODULE_2]);
    Serial.print(F("NSS: GPIO"));
    Serial.print(nssPins[MODULE_2]);
    Serial.print(F(", DIO0: GPIO"));
    Serial.print(dio0Pins[MODULE_2]);
    Serial.print(F(", RST: GPIO"));
    Serial.println(resetPins[MODULE_2]);

    Serial.println(F("================================"));
}
