#include "DummySensors.h"
#include "MessageProtocol.h"  // For sensor ID constants

DummySensors::DummySensors() {
    // Initialize base values
    temperatureBase = 25.0;
    humidityBase = 60.0;
    batteryBase = 3.7;
    pressureBase = 1013.25;
}

void DummySensors::begin() {
    // Seed random with unique value
    randomSeed(analogRead(0) + micros());

    // Randomize initial base values within range
    temperatureBase = random(2000, 3000) / 100.0;  // 20-30°C
    humidityBase = random(4000, 8000) / 100.0;     // 40-80%
    batteryBase = random(330, 420) / 100.0;        // 3.3-4.2V
    pressureBase = random(98000, 102000) / 100.0;  // 980-1020 hPa
}

float DummySensors::addVariation(float base, float minVal, float maxVal, float variationAmount) {
    // Add small random variation to base value
    float variation = random(-100, 101) / 100.0 * variationAmount;
    float newValue = base + variation;

    // Keep within bounds
    if (newValue < minVal) newValue = minVal;
    if (newValue > maxVal) newValue = maxVal;

    return newValue;
}

float DummySensors::readTemperature() {
    // Slowly drift temperature
    temperatureBase = addVariation(temperatureBase, 20.0, 30.0, 0.5);
    return temperatureBase;
}

float DummySensors::readHumidity() {
    // Slowly drift humidity
    humidityBase = addVariation(humidityBase, 40.0, 80.0, 1.0);
    return humidityBase;
}

float DummySensors::readBatteryVoltage() {
    // Slowly drift battery (typically decreasing)
    batteryBase = addVariation(batteryBase, 3.3, 4.2, 0.02);
    return batteryBase;
}

float DummySensors::readPressure() {
    // Slowly drift pressure
    pressureBase = addVariation(pressureBase, 980.0, 1020.0, 2.0);
    return pressureBase;
}

float DummySensors::readSensorById(uint8_t sensorId) {
    switch (sensorId) {
        case SENSOR_TEMPERATURE:
            return readTemperature();
        case SENSOR_HUMIDITY:
            return readHumidity();
        case SENSOR_BATTERY:
            return readBatteryVoltage();
        case SENSOR_PRESSURE:
            return readPressure();
        default:
            return 0.0;
    }
}

const char* DummySensors::getSensorName(uint8_t sensorId) {
    switch (sensorId) {
        case SENSOR_TEMPERATURE: return "Temperature";
        case SENSOR_HUMIDITY: return "Humidity";
        case SENSOR_BATTERY: return "Battery";
        case SENSOR_PRESSURE: return "Pressure";
        default: return "Unknown";
    }
}

const char* DummySensors::getSensorUnit(uint8_t sensorId) {
    switch (sensorId) {
        case SENSOR_TEMPERATURE: return "°C";
        case SENSOR_HUMIDITY: return "%";
        case SENSOR_BATTERY: return "V";
        case SENSOR_PRESSURE: return "hPa";
        default: return "";
    }
}
