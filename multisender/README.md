# LoRa Ra-02 Multi-Sender

Dual LoRa module transmitter - sends sensor data from two LoRa modules (trident1, trident2) on a single ESP32.

## Features

- Two LoRa Ra-02 (SX1278) modules on one ESP32
- Alternating transmission between modules
- Device identification in each packet
- Rotates through all sensor types (Temperature, Humidity, Battery, Pressure)

## Supported Boards

- ESP32 DevKit V1
- Arduino Nano ESP32

**Note:** Arduino UNO is not supported due to limited RAM (2KB) and single SPI bus.

## Wiring Diagram

### ESP32 DevKit + 2 LoRa Modules

```
ESP32 DevKit          LoRa Module 1 (trident1)     LoRa Module 2 (trident2)
-----------           ----------------------        ----------------------
GPIO 5  (NSS1)  ----> NSS
GPIO 15 (NSS2)  ----------------------------->     NSS
GPIO 14 (DIO0_1) ---> DIO0
GPIO 27 (DIO0_2) ---------------------------->     DIO0
GPIO 21 (RST1)  ---> RESET
GPIO 26 (RST2)  ----------------------------->     RESET
GPIO 18 (SCK)   ----> SCK  ------------------>     SCK    (shared)
GPIO 19 (MISO)  <---- MISO <------------------     MISO   (shared)
GPIO 23 (MOSI)  ----> MOSI ------------------>     MOSI   (shared)
3.3V            ----> VCC  ------------------>     VCC
GND             ----> GND  ------------------>     GND
```

### Arduino Nano ESP32 + 2 LoRa Modules

```
Nano ESP32            LoRa Module 1 (trident1)     LoRa Module 2 (trident2)
----------            ----------------------        ----------------------
D10 (NSS1)      ----> NSS
D7  (NSS2)      ----------------------------->     NSS
D2  (DIO0_1)    ---> DIO0
D3  (DIO0_2)    ---------------------------->     DIO0
D9  (RST1)      ---> RESET
D8  (RST2)      ----------------------------->     RESET
D13 (SCK)       ----> SCK  ------------------>     SCK    (shared)
D12 (MISO)      <---- MISO <------------------     MISO   (shared)
D11 (MOSI)      ----> MOSI ------------------>     MOSI   (shared)
3.3V            ----> VCC  ------------------>     VCC
GND             ----> GND  ------------------>     GND
```

## Pin Summary Table

| Signal | ESP32 DevKit | Nano ESP32 | Module 1 | Module 2 |
|--------|--------------|------------|----------|----------|
| NSS    | GPIO 5       | D10        | Yes      | -        |
| NSS    | GPIO 15      | D7         | -        | Yes      |
| DIO0   | GPIO 14      | D2         | Yes      | -        |
| DIO0   | GPIO 27      | D3         | -        | Yes      |
| RESET  | GPIO 21      | D9         | Yes      | -        |
| RESET  | GPIO 26      | D8         | -        | Yes      |
| SCK    | GPIO 18      | D13        | Shared   | Shared   |
| MISO   | GPIO 19      | D12        | Shared   | Shared   |
| MOSI   | GPIO 23      | D11        | Shared   | Shared   |

## Build & Upload

```bash
# Build for all boards
pio run

# Build for specific board
pio run -e esp32dev
pio run -e nano_esp32

# Upload to ESP32 DevKit
pio run -e esp32dev -t upload

# Monitor serial output
pio device monitor
```

## Expected Output

```
==========================================
  LoRa Ra-02 MULTI-SENDER
  Dual Module Transmitter
==========================================
Board: ESP32Dev
Module 1: trident1
Module 2: trident2

=== Dual LoRa Module Initialization ===
...
=== Both modules initialized successfully ===

[TX] [trident1] Temperature: 25.34 °C (20 bytes)
[TX] [trident2] Humidity: 65.20 % (18 bytes)
[TX] [trident1] Battery: 3.85 V (17 bytes)
[TX] [trident2] Pressure: 1013.25 hPa (21 bytes)
```

## Receiver Output

When using the updated receiver, you'll see:

```
[5s] [trident1] Temperature: 25.34 °C | RSSI: -35 dBm | SNR: 9.5 dB | ID: 1
[10s] [trident2] Humidity: 65.20 % | RSSI: -33 dBm | SNR: 9.8 dB | ID: 2
[15s] [trident1] Battery: 3.85 V | RSSI: -36 dBm | SNR: 9.2 dB | ID: 3
```

## Customizing Device Names

Edit `platformio.ini` to change device names:

```ini
build_flags =
    ...
    -D LORA1_NAME=\"mydevice1\"
    -D LORA2_NAME=\"mydevice2\"
```
