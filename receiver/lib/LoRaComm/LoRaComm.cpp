#include "LoRaComm.h"
#include <board_config.h>

LoRaComm::LoRaComm() : lastRSSI(0), lastSNR(0.0) {
}

bool LoRaComm::begin() {
    Serial.println(F("\n=== LoRa Initialization Debug ==="));

    // Print pin configuration
    Serial.print(F("NSS (CS): GPIO "));
    Serial.println(LORA_NSS);
    Serial.print(F("RESET: GPIO "));
    Serial.println(LORA_RESET);
    Serial.print(F("DIO0: GPIO "));
    Serial.println(LORA_DIO0);

    #if defined(LORA_SCK) && defined(LORA_MISO) && defined(LORA_MOSI)
        Serial.print(F("SCK: GPIO "));
        Serial.println(LORA_SCK);
        Serial.print(F("MISO: GPIO "));
        Serial.println(LORA_MISO);
        Serial.print(F("MOSI: GPIO "));
        Serial.println(LORA_MOSI);
    #endif

    Serial.print(F("Frequency: "));
    Serial.print(LORA_FREQUENCY / 1E6);
    Serial.println(F(" MHz"));

    // Initialize custom SPI pins for ESP32 Dev Board if defined
    #if defined(LORA_SCK) && defined(LORA_MISO) && defined(LORA_MOSI)
        Serial.println(F("\nInitializing custom SPI pins..."));
        SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
        Serial.println(F("Custom SPI initialized"));
        delay(50);
    #else
        Serial.println(F("\nUsing default SPI pins"));
    #endif

    // Set LoRa pins (NSS, RESET, DIO0)
    Serial.println(F("Setting LoRa pins..."));
    LoRa.setPins(LORA_NSS, LORA_RESET, LORA_DIO0);

    // Add delay after reset to let module stabilize
    Serial.println(F("Performing hardware reset..."));
    pinMode(LORA_RESET, OUTPUT);
    digitalWrite(LORA_RESET, LOW);
    delay(10);
    digitalWrite(LORA_RESET, HIGH);
    delay(100);
    Serial.println(F("Reset complete, waiting for module..."));

    // Initialize LoRa module with frequency
    Serial.println(F("Attempting LoRa.begin()..."));
    Serial.print(F("Reading SX1278 version register..."));
    if (!LoRa.begin(LORA_FREQUENCY)) {
        Serial.println(F("\n!!! ERROR: LoRa initialization failed !!!"));
        Serial.println(F("\nPossible causes:"));
        Serial.println(F("  1. Wiring issues:"));
        Serial.println(F("     - Check SPI connections (NSS, MOSI, MISO, SCK)"));
        Serial.println(F("     - Verify GND connection"));
        Serial.println(F("     - Verify 3.3V power (NOT 5V!)"));
        Serial.println(F("  2. Module issues:"));
        Serial.println(F("     - LoRa module not powered"));
        Serial.println(F("     - Damaged SX1278 chip"));
        Serial.println(F("     - Wrong module (not Ra-02/SX1278)"));
        Serial.println(F("  3. SPI bus conflict:"));
        Serial.println(F("     - Another device using same pins"));
        Serial.println(F("     - Check if pins are already in use"));
        Serial.println(F("  4. Pin configuration:"));
        Serial.println(F("     - Verify GPIO numbers match your wiring"));
        Serial.println(F("     - ESP32: GPIO34-39 are INPUT ONLY"));
        Serial.println(F("\nDouble-check your wiring against the pin numbers above!"));
        return false;
    }

    Serial.println(F("LoRa.begin() succeeded!"));

    // Configure LoRa parameters
    LoRa.setSpreadingFactor(LORA_SPREADING_FACTOR);
    LoRa.setSignalBandwidth(LORA_SIGNAL_BANDWIDTH);
    LoRa.setCodingRate4(LORA_CODING_RATE);
    LoRa.setPreambleLength(LORA_PREAMBLE_LENGTH);
    LoRa.setSyncWord(LORA_SYNC_WORD);
    LoRa.setTxPower(LORA_TX_POWER);

    // Enable CRC
    LoRa.enableCrc();

    Serial.println(F("SUCCESS: LoRa module initialized"));
    printConfig();

    return true;
}

bool LoRaComm::sendPacket(const uint8_t* data, size_t length) {
    if (length == 0 || length > 255) {
        Serial.println(F("ERROR: Invalid packet length"));
        return false;
    }

    // Begin packet
    LoRa.beginPacket();

    // Write data
    LoRa.write(data, length);

    // End packet and transmit
    if (!LoRa.endPacket()) {
        Serial.println(F("ERROR: Packet transmission failed"));
        return false;
    }

    return true;
}

int LoRaComm::receivePacket(uint8_t* buffer, size_t maxLength) {
    int packetSize = LoRa.parsePacket();

    if (packetSize == 0) {
        return 0;  // No packet available
    }

    // Store RSSI and SNR
    lastRSSI = LoRa.packetRssi();
    lastSNR = LoRa.packetSnr();

    // Read packet data
    int bytesRead = 0;
    while (LoRa.available() && (size_t)bytesRead < maxLength) {
        buffer[bytesRead++] = LoRa.read();
    }

    return bytesRead;
}

bool LoRaComm::isPacketAvailable() {
    return LoRa.parsePacket() > 0;
}

int LoRaComm::getRSSI() {
    return lastRSSI;
}

float LoRaComm::getSNR() {
    return lastSNR;
}

bool LoRaComm::isTransmitting() {
    // Note: arduino-LoRa library doesn't expose this directly
    // For now, we return false (blocking transmission mode)
    return false;
}

void LoRaComm::onReceive(void (*callback)(int)) {
    LoRa.onReceive(callback);
}

void LoRaComm::printConfig() {
    Serial.println(F("--- LoRa Configuration ---"));
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

    Serial.print(F("Pins - NSS: "));
    Serial.print(LORA_NSS);
    Serial.print(F(", DIO0: "));
    Serial.print(LORA_DIO0);
    Serial.print(F(", RST: "));
    Serial.println(LORA_RESET);

    #if defined(LORA_SCK) && defined(LORA_MISO) && defined(LORA_MOSI)
        Serial.print(F("SPI - SCK: "));
        Serial.print(LORA_SCK);
        Serial.print(F(", MISO: "));
        Serial.print(LORA_MISO);
        Serial.print(F(", MOSI: "));
        Serial.println(LORA_MOSI);
    #endif

    Serial.println(F("-------------------------"));
}
