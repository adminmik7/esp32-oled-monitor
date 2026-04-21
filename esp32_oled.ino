#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1 

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int cpu = 0; 
int ram = 0;
unsigned long lastUpdate = 0;

char input[32]; 
byte idx = 0;   

void setup() {
  Serial.begin(115200);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    return; 
  }
  
  display.clearDisplay();
  display.setTextSize(1); 
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("  PC Monitor");
  display.println("  ESP32 + OLED");
  display.display();
  
  lastUpdate = millis() + 5000; 
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

  if (millis() - lastUpdate >= 5000) {
    showWait();
    return; 
  } else {
    updateScreen();
  }
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
  
  // --- CPU Блок ---
  display.setTextSize(2); 
  display.setCursor(0, 0);
  display.print("CPU: ");
  display.print(cpu);
  display.print("%");
  
  // Бар CPU (Y=18)
  display.fillRect(0, 18, map(cpu, 0, 100, 0, 128), 8, SSD1306_WHITE);
  

  // --- RAM Блок ---
  // Старт текста RAM: Y=32 (с учетом отступа от предыдущего блока)
  display.setCursor(0, 32); 
  
  display.print("RAM: ");
  display.print(ram);
  display.print("%");
  
  // РАСЧЁТ ПОЛОЗЕНИЯ БАРА RAM:
  // Y текста (32) + Высота шрифта (~10-11 пикселей) + Отступ (4) = ~46.
  // ВАША ПРАВКА: Опускаем еще на 2 пикселя -> 48.
  int ramBarY = 51; 
  
  display.fillRect(0, ramBarY, map(ram, 0, 100, 0, 128), 8, SSD1306_WHITE);
  
  display.display();
}

void showWait() {
  display.clearDisplay();
  display.setTextSize(2);
  
  // Центрируем текст и опускаем на уровень ниже середины экрана
  // (Текст шрифтом 2 занимает ~11-12px, высота экрана 64. Центр Y=32.)
  display.setCursor((128 - 45) / 2, 34); 
  display.print("WAIT");
  
  display.display();
}
