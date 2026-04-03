#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int cpu = 0, ram = 0;
unsigned long lastUpdate = 0;
char input[32];
byte idx = 0;

void setup() {
  Serial.begin(115200);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("  PC Monitor");
  display.println("  ESP32 + OLED");
  display.display();
  delay(2000);
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      input[idx] = 0;
      parseInput();
      idx = 0;
    } else if (idx < 31) {
      input[idx++] = c;
    }
  }

  if (millis() - lastUpdate > 5000) {
    showWait();
    return;
  }

  updateScreen();
}

void parseInput() {
  char* cPtr = strstr(input, "CPU:");
  char* rPtr = strstr(input, "RAM:");
  
  if (cPtr) cpu = atoi(cPtr + 4);
  if (rPtr) ram = atoi(rPtr + 4);
  
  lastUpdate = millis();
  Serial.println("OK");
}

void updateScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print("CPU: ");
  display.print(cpu);
  display.print("%");
  
  display.setCursor(0, 24);
  display.print("RAM: ");
  display.print(ram);
  display.print("%");

  // Простые бары
  display.fillRect(0, 48, map(cpu, 0, 100, 0, 64), 8, SSD1306_WHITE);
  display.fillRect(0, 56, map(ram, 0, 100, 0, 64), 8, SSD1306_WHITE);
  
  display.display();
}

void showWait() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(30, 28);
  display.print("WAIT");
  display.display();
}
