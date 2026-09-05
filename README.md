# IoT-Based Poultry House Monitoring System

This repository contains the ESP32 firmware recovered from the user's Claude export for the IoT-based poultry house monitoring project.

## Recovered source
- The file `firmware/poultry_monitor/poultry_monitor_latest_recovered.ino` is the latest complete firmware command found in the exported conversation on 2026-09-04.
- It preserves the recovered Claude version, including Wi-Fi, Firebase, LCD, sensors, LEDs, buzzer, and SIM800L GSM fallback.
- `poultry_monitor_firebase_only.ino` is a separate Firebase-only derivative prepared from that recovered version by removing the GSM fallback.

## Hardware
- ESP32-32D DevKitC V4
- DHT22
- MQ-135
- LDR module
- 16x2 I2C LCD (0x27)
- SIM800L V2.0 (used only by the recovered dual-communication version)
- 2 red LEDs, 1 yellow LED, 1 green LED
- Active buzzer

## GPIO assignments
| GPIO | Component |
|---:|---|
| 4 | DHT22 Data |
| 34 | MQ-135 Analog |
| 35 | LDR Analog |
| 21 | LCD SDA |
| 22 | LCD SCL |
| 14 | SIM800L TXD (recovered dual version) |
| 27 | SIM800L RXD (recovered dual version) |
| 25 | Red LED - Temperature |
| 26 | Red LED - Ammonia |
| 32 | Yellow LED - Humidity |
| 33 | Green LED - System OK |
| 23 | Active buzzer |

## Thresholds in recovered firmware
- Temperature alert: above 30.0 °C
- Humidity alert: above 75.0% or below 40.0%
- Ammonia alert: above 25.0 ppm
- Monitoring interval: 30 seconds

## LED and buzzer behavior
- Red LED 1: temperature alert
- Red LED 2: ammonia alert
- Yellow LED: humidity outside range
- Green LED: all monitored parameters normal
- Buzzer: temperature or ammonia alert only

## Firebase paths
```text
/sensors/temperature
/sensors/humidity
/sensors/ammonia_ppm
/sensors/light
/sensors/status
/alerts/latest
```

## Required Arduino libraries
See `libraries/required_libraries.txt`.

## Credentials
Do **not** commit real Wi-Fi passwords, Firebase credentials, or phone numbers. Replace the placeholders in the firmware locally or move secrets to a separate ignored configuration file before publishing a public repository.
