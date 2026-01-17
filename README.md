1# LoRa Ra-02 SX1278 Firmware Collection

Complete firmware collection for LoRa Ra-02 SX1278 modules supporting Arduino Nano ESP32, ESP32 Dev Board, and Arduino Uno.

## 📁 Projects

### 1. **bidirectional-master/** - Full-Featured Bidirectional Communication
Production-grade request-response firmware with ACKs, retries, and serial commands.

**Features:**
- Request-response pattern with acknowledgments
- Automatic retries with exponential backoff
- Text messages, sensor requests, commands
- Serial command interface for testing
- Statistics tracking

**Use Case:** Interactive testing, full protocol demonstration, production applications

**Upload:**
```bash
cd bidirectional-master
pio run -e esp32dev -t upload        # ESP32 Dev Board
pio run -e nano_esp32 -t upload      # Arduino Nano ESP32
```

---

### 2. **sender/** - Simple Sensor Data Transmitter
Minimal firmware that sends dummy sensor data every 5 seconds.

**Features:**
- Auto-transmit every 5 seconds
- Rotates through sensors (Temp → Humidity → Battery → Pressure)
- No user interaction needed
- Lightweight and simple

**Use Case:** Quick testing, one-way data transmission, sensor node simulation

**Upload:**
```bash
cd sender
pio run -e esp32dev -t upload        # ESP32 Dev Board
pio run -e nano_esp32 -t upload      # Arduino Nano ESP32
```

**Output Example:**
```
[TX] Temperature: 25.34 °C (12 bytes)
[TX] Humidity: 65.20 % (12 bytes)
[TX] Battery: 3.87 V (12 bytes)
```

---

### 3. **receiver/** - Simple Data Receiver
Minimal firmware that listens and displays received sensor data.

**Features:**
- Continuously listens for packets
- Displays sensor data with RSSI/SNR
- Shows text messages
- Statistics every 20 messages

**Use Case:** Quick testing, data logging, monitoring remote sensors

**Upload:**
```bash
cd receiver
pio run -e esp32dev -t upload        # ESP32 Dev Board
pio run -e nano_esp32 -t upload      # Arduino Nano ESP32
```

**Output Example:**
```
[15s] Temperature: 25.34 °C | RSSI: -35 dBm | SNR: 9.5 dB | ID: 42
[20s] Humidity: 65.20 % | RSSI: -33 dBm | SNR: 9.8 dB | ID: 43
```

---

## 🔧 Hardware Setup

All three projects use the same pin configuration.

### Quick Pin Reference

| Board | NSS | DIO0 | RESET | MOSI | MISO | SCK | SPI Type |
|-------|-----|------|-------|------|------|-----|----------|
| **Arduino Nano ESP32** | D10 | D2 | D9 | D11 | D12 | D13 | Hardware SPI (default) |
| **ESP32 Dev** | GPIO5 | GPIO14 | GPIO21 | GPIO23 | GPIO19 | GPIO18 | VSPI (default) |
| **Arduino Uno** | Pin 10 | Pin 2 | Pin 9 | Pin 11 | Pin 12 | Pin 13 | Hardware SPI (via level shifter) |

### Wiring Diagrams

#### Arduino Nano ESP32 to Ra-02

```
Ra-02 Module       Arduino Nano ESP32
──────────────────────────────────────
GND (Pin 1)    →   GND
VCC (Pin 2)    →   3.3V (+ 10µF cap)
DIO0 (Pin 3)   →   D2
NSS (Pin 12)   →   D10
MOSI (Pin 11)  →   D11 [Hardware SPI]
MISO (Pin 10)  →   D12 [Hardware SPI]
SCK (Pin 9)    →   D13 [Hardware SPI]
RESET (Pin 13) →   D9
ANT (Pin 14)   →   433 MHz Antenna
```

#### ESP32 Dev Board to Ra-02

```
Ra-02 Module       ESP32 Dev Board
──────────────────────────────────────
GND (Pin 1)    →   GND
VCC (Pin 2)    →   3.3V (+ 10µF cap)
DIO0 (Pin 3)   →   GPIO14 (D14)
NSS (Pin 12)   →   GPIO5 (D5)
MOSI (Pin 11)  →   GPIO23 (D23) [VSPI]
MISO (Pin 10)  →   GPIO19 (D19) [VSPI]
SCK (Pin 9)    →   GPIO18 (D18) [VSPI]
RESET (Pin 13) →   GPIO21 (D21)
ANT (Pin 14)   →   433 MHz Antenna
```

**⚠️ IMPORTANT:** DO NOT use GPIO34-39 for DIO0 or RESET - these are INPUT ONLY pins on ESP32!

---

## 🚀 Quick Start

### Test Setup (Recommended)

**Option 1: Sender ↔ Receiver (Simplest)**
1. Upload `sender` to Board A
2. Upload `receiver` to Board B
3. Open serial monitor on both
4. Watch data flow automatically

**Option 2: Bidirectional ↔ Bidirectional (Full Featured)**
1. Upload `bidirectional-master` to both boards
2. Open serial monitors
3. Use commands: `send hi`, `request temp`, `stats`

**Option 3: Mixed**
- Board A: `sender` (auto-transmit)
- Board B: `bidirectional-master` (can respond)

### Build All Projects

```bash
# From root directory
cd bidirectional-master && pio run && cd ..
cd sender && pio run && cd ..
cd receiver && pio run && cd ..
```

---

## 📡 LoRa Configuration

All projects use identical LoRa settings (configured in `include/board_config.h`):

```c
Frequency: 433 MHz
Spreading Factor: 7 (SF7)
Bandwidth: 125 kHz
Coding Rate: 4/5
TX Power: 17 dBm
Sync Word: 0x12 (private network)
```

**Range Estimates:**
- SF7: ~2 km (line of sight)
- SF12: ~15 km (line of sight)

---

## 🐛 Troubleshooting

### LoRa initialization failed

**ESP32 Specific:**
- Verify DIO0 is on GPIO14 (NOT GPIO34-39)
- GPIO34-39 are INPUT ONLY and won't work

**General:**
- Check all SPI connections (NSS, MOSI, MISO, SCK)
- Verify 3.3V power supply
- Ensure antenna is connected
- Add 10-100µF capacitor near Ra-02 VCC

### No messages received

- Verify both boards use same frequency (433 MHz)
- Check sync word matches (0x12)
- Reduce distance (<5m for initial testing)
- Check RSSI values (should be > -120 dBm)

### Permission errors during upload

```bash
sudo chmod 666 /dev/ttyUSB1    # ESP32 Dev
sudo chmod 666 /dev/ttyACM1    # Arduino Nano ESP32
```

---

## 📊 Recent Updates

### ✅ v1.2 - Project Restructure (2026-01-12)
- Split into three projects: bidirectional-master, sender, receiver
- Simplified testing workflow
- Added dedicated sender/receiver firmware

### ✅ v1.1 - Retry Bug Fix
- Fixed retry packet corruption (was sending 260 bytes instead of actual length)
- Added 100ms delay between ACK and sensor response
- Retries now work reliably

### ✅ v1.0 - GPIO Pin Fix
- Fixed ESP32 GPIO34-39 input-only issue
- Switched to default VSPI pins for reliability

---

## 📖 Documentation

- **bidirectional-master/README.md** - Full protocol documentation
- **sender/src/main.cpp** - Sender source code
- **receiver/src/main.cpp** - Receiver source code

---

## 🛠️ Development

```bash
# Build specific project and board
cd sender
pio run -e esp32dev

# Upload
pio run -e esp32dev -t upload

# Monitor serial output
pio device monitor -p /dev/ttyUSB1 --baud 9600
```

---

## 📝 License

MIT License - Free to use and modify

---

## 🎯 Which Project Should I Use?

| Scenario | Recommended Project |
|----------|-------------------|
| Quick testing | **sender** + **receiver** |
| Learning LoRa | **sender** + **receiver** |
| Production sensor nodes | **sender** or **bidirectional-master** |
| Full protocol testing | **bidirectional-master** |
| One-way data logging | **sender** + **receiver** |
| Interactive control | **bidirectional-master** |

---

**Happy LoRa-ing! 📡**
