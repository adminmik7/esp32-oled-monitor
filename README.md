# 📊 esp32-oled-monitor — ESP32 + OLED PC Monitor

**Windows Edition.** Отображение CPU и RAM на OLED экране 0.96" (128x64).

## 🔌 Подключение

| ESP32 | OLED 0.96" (I2C) |
|---|---|
| 3V3 | VCC |
| GND | GND |
| GPIO 21 (SDA) | SDA |
| GPIO 22 (SCL) | SCL |

## 🚀 Запуск в Windows

1.  Установи **Python** (не забудь галочку "Add to PATH").
2.  Скачай этот репозиторий.
3.  Дважды кликни по **`start.bat`**.

Скрипт сам найдет ESP32, установит библиотеки и начнет отправку данных.

## 📦 Зависимости для Arduino IDE
- `Adafruit SSD1306`
- `Adafruit GFX Library`
