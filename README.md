# MiWP_AiRP_Project

## About

Studies project for monitoring and visualization of processes + automatization and robotization of proceses

Used hardware:

- NodeMCU ESP8266
- DHT11
- BME280 (pressure sesor)
- MQ7 gas sensor
- MQ2 gas sensor (optional)

Used technologies and sofware:

- Arduino IDE
- [Adafruit BMP280 Library](https://github.com/adafruit/Adafruit_BMP280_Library)
- [Arduinojson](https://arduinojson.org/?utm_source=meta&utm_medium=library.properties)
- [MQUnifiedsensor](https://github.com/miguel5612/MQSensorsLib)
- [esp826611](https://github.com/adafruit/DHT-sensor-library)
- Python 3
- [FastAPI](https://fastapi.tiangolo.com/)
- PostgreSQL
- Docker

## Project pinout

| Usage (name) | Pin board | Pin component |
|---|---|---|
| I2C_SCL | D1 | `BME 280` SCL |
| I2C_SDA | D2 | `BME 280` SDA |
| DHT_PIN | D4 | `DHT` OUT |
| MQ7_PIN | A0 | `MQ7` A0 |
| MQ2_PIN | A0 | `MQ2` A0 |
| VCC | VCC (3v3) | VCC |
| GND | GNG | GND |
