#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <LittleFS.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1 

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int gpu = 0;
int ram = 0;
unsigned long lastUpdate = 0;
bool dataReceived = false;
unsigned long lastDataTime = 0;
unsigned long lastDataTs = 0;
unsigned long blinkTimer = 0;
bool blinkState = false;

char input[256]; 
byte idx = 0;

Preferences wifiPrefs;
String storedSsid = "a1_136";
String storedPassword = "14042010";
unsigned long wifiConnectStart = 0;
bool wifiConnected = false;

WebServer server(80);   

void loadWifiSettings();
void saveWifiSettings(const char* ssid, const char* pass);
void parseInput();
void updateScreen();
void showNoDataBlink();

void setup() {
  Serial.begin(115200);
  
  // Инициализация файловой системы LittleFS
  if(!LittleFS.begin(true)){
    Serial.println("LittleFS mount failed, formatting...");
    LittleFS.format();
    Serial.println("LittleFS formatted");
  }
  Serial.println("LittleFS mounted OK");
  
  loadWifiSettings();
  
  WiFi.begin(storedSsid.c_str(), storedPassword.c_str());
  Serial.print("Connecting to WiFi");
  wifiConnectStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - wifiConnectStart > 30000) {
      Serial.println("");
      Serial.println("WiFi connect timeout, using defaults");
      storedSsid = "a1_136";
      storedPassword = "14042010";
      WiFi.begin(storedSsid.c_str(), storedPassword.c_str());
      wifiConnectStart = millis();
    }
  }
  wifiConnected = true;
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    return; 
  }
  
  display.clearDisplay();
  display.setTextSize(1); 
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.display();
  
  server.on("/status", []() {
    String json = "{\"gpu\":" + String(gpu) + ",\"ram\":" + String(ram) + ",\"ts\":" + String(lastDataTs) + "}";
    server.send(200, "application/json", json);
  });

  // API для получения текущих настроек WiFi (для страницы настроек)
  server.on("/api/settings", []() {
    String json = "{\"ssid\":\"" + storedSsid + "\",\"pass\":\"" + storedPassword + "\"}";
    server.send(200, "application/json", json);
  });

  // Страница настроек - читается из LittleFS
  server.on("/settings", []() {
    File file = LittleFS.open("/settings.html", "r");
    if(file){
      server.streamFile(file, "text/html");
      file.close();
    } else {
      server.send(500, "text/plain", "Settings file not found");
    }
  });
  
  server.on("/settings", HTTP_POST, []() {
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
    if (ssid.length() > 0 && pass.length() > 0) {
      saveWifiSettings(ssid.c_str(), pass.c_str());
      server.sendHeader("Location", "/");
      server.send(302);
      delay(100);
      ESP.restart();
    } else {
      server.sendHeader("Location", "/settings");
      server.send(302);
    }
  });
  
  // Главная страница - читается из LittleFS
  server.on("/", []() {
    File file = LittleFS.open("/index.html", "r");
    if(file){
      server.streamFile(file, "text/html");
      file.close();
    } else {
      server.send(500, "text/plain", "Index file not found");
    }
  });
  
  server.begin();
  Serial.println("HTTP server started");
  
 lastUpdate = millis() + 3000;
  delay(2000); 
}

void loop() {
  server.handleClient();
  
  while (Serial.available()) {
    char c = Serial.read();
    
    if (c == '\n') {
      input[idx] = 0; 
      parseInput();
      idx = 0;
      memset(input, 0, sizeof(input));
    } else if (idx < 255) {
      input[idx++] = c;
    }
  }

  if (millis() - lastUpdate >= 3000) {
    showNoDataBlink();
  } else {
    updateScreen();
  }
}

void loadWifiSettings() {
  wifiPrefs.begin("monitor");
  storedSsid = wifiPrefs.getString("ssid", "a1_136");
  storedPassword = wifiPrefs.getString("pass", "14042010");
  wifiPrefs.end();
}

void saveWifiSettings(const char* ssid, const char* pass) {
  wifiPrefs.begin("monitor");
  wifiPrefs.putString("ssid", ssid);
  wifiPrefs.putString("pass", pass);
  wifiPrefs.end();
  storedSsid = ssid;
  storedPassword = pass;
}

void parseInput() {
  char buf[256];
  strncpy(buf, input, sizeof(buf));
  buf[sizeof(buf)-1] = 0;
  
  char* line = strtok(buf, "\n");
  while (line) {
    char* gPtr = strstr(line, "GPU:");
    char* rPtr = strstr(line, "RAM:");
    char* tPtr = strstr(line, "TS:");
    
    if (gPtr) gpu = atoi(gPtr + 4);
    if (rPtr) ram = atoi(rPtr + 4);
    if (tPtr) lastDataTs = strtoul(tPtr + 3, nullptr, 10);
    
    line = strtok(nullptr, "\n");
  }
  
  if (gpu >= 0 || ram >= 0) {
    dataReceived = true;
    lastUpdate = millis();
    lastDataTime = millis();
  }
  Serial.println("OK");
}

void updateScreen() {
  if (!dataReceived) {
    display.clearDisplay();
    display.display();
    return;
  }
  
  display.clearDisplay();
  
  // --- GPU Блок ---
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print("GPU: ");
  display.print(gpu);
  display.print("%");

  // Бар GPU (Y=16)
  display.fillRect(0, 16, map(gpu, 0, 100, 0, 128), 12, SSD1306_WHITE);
  

  // --- RAM Блок ---
  // Старт текста RAM: Y=32 (с учетом отступа от предыдущего блока)
  display.setCursor(0, 32); 
  
  display.print("RAM: ");
  display.print(ram);
  display.print("%");
  
  // Бар RAM
  int ramBarY = 51;

  display.fillRect(0, ramBarY - 2, map(ram, 0, 100, 0, 128), 12, SSD1306_WHITE);
  
  display.display();
}

void showNoDataBlink() {
  if (millis() - blinkTimer >= 500) {
    blinkState = !blinkState;
    blinkTimer = millis();
  }
  
  display.clearDisplay();
  
  if (blinkState) {
    display.fillRect(120, 0, 5, 5, SSD1306_WHITE);
  }
  
  display.display();
}

