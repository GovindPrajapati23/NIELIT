#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <Adafruit_BMP280.h>

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

// ─── OLED ───────────────────────────────────────
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ─── DHT11 ──────────────────────────────────────
#define DHTPIN 2        // D4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ─── BMP280 ─────────────────────────────────────
Adafruit_BMP280 bmp;

// ─── Other Pins ─────────────────────────────────
#define LDR_PIN   14    // D5 — digital
#define RAIN_PIN  12    // D6 — digital
#define MQ135_PIN A0    // analog

const char* ssid = "realme 9 5G Speed Edition";
const char* password = "b2e566gh";

ESP8266WebServer server(80);
void handleReadADC();

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // ── OLED Init ──
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("ERROR: OLED not found!");
    while (true);
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  Serial.println("OLED OK");

  // ── BMP280 Init ──
  if (!bmp.begin(0x76)) {   // try 0x77 if this fails
    Serial.println("ERROR: BMP280 not found!");
    while (true);
  }
  Serial.println("BMP280 OK");

  // ── DHT11 Init ──
  dht.begin();
  Serial.println("DHT11 OK");

  // ── Pin Modes ──
  pinMode(LDR_PIN,  INPUT);
  pinMode(RAIN_PIN, INPUT);

  Serial.println("─────────────────────────────────");
  Serial.println("   Weather Station Starting...   ");
  Serial.println("─────────────────────────────────");

  // Splash screen
  display.setTextSize(1);
  display.setCursor(15, 10); display.println("WEATHER STATION");
  display.setCursor(25, 30); display.println("Initializing");
  display.setCursor(35, 45); display.println("Please wait");
  display.display();
  delay(2000);

// Connect WiFi
WiFi.begin(ssid, password);

Serial.print("Connecting to WiFi");

while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  Serial.print(".");
}

Serial.println();
Serial.println("WiFi Connected");
Serial.print("IP Address: ");
Serial.println(WiFi.localIP());

if (!LittleFS.begin()) {
  Serial.println("LittleFS Mount Failed");
} else {
  Serial.println("LittleFS Mounted");
}

server.serveStatic("/", LittleFS, "/index.html");
server.serveStatic("/style.css", LittleFS, "/style.css");
server.serveStatic("/script.js", LittleFS, "/script.js");

server.on("/readADC", handleReadADC); 

server.begin();
Serial.println("HTTP Server Started");
}




void loop() {

  server.handleClient();

  // ════════════════════════════════════
  //        READ ALL SENSORS
  // ════════════════════════════════════
  float humidity   = dht.readHumidity();
  float tempDHT    = dht.readTemperature();
  float tempBMP    = bmp.readTemperature();
  float pressure   = bmp.readPressure() / 100.0F;  // hPa
  float altitude   = bmp.readAltitude(1013.25);
  int   airQuality = analogRead(MQ135_PIN);
  bool  isDark     = !digitalRead(LDR_PIN);    // LOW = dark
  bool  isRaining  = !digitalRead(RAIN_PIN);   // LOW = rain

  // Validate DHT11 (returns NaN if read fails)
  if (isnan(humidity) || isnan(tempDHT)) {
    Serial.println("DHT11 READ FAILED — retrying...");
    delay(2000);
    return;
  }

  // ════════════════════════════════════
  //        SERIAL MONITOR OUTPUT
  // ════════════════════════════════════
  Serial.println("┌─────────────────────────────┐");
  Serial.println("│    SENSOR READINGS           │");
  Serial.println("├─────────────────────────────┤");
  Serial.print("│ Temp (DHT11) : ");
  Serial.print(tempDHT, 1);
  Serial.println(" C          │");
  Serial.print("│ Temp (BMP280): ");
  Serial.print(tempBMP, 1);
  Serial.println(" C          │");
  Serial.print("│ Humidity     : ");
  Serial.print(humidity, 1);
  Serial.println(" %          │");
  Serial.print("│ Pressure     : ");
  Serial.print(pressure, 1);
  Serial.println(" hPa        │");
  Serial.print("│ Altitude     : ");
  Serial.print(altitude, 1);
  Serial.println(" m          │");
  Serial.print("│ Air Quality  : ");
  Serial.print(airQuality);
  Serial.print("  → ");
  Serial.println(aqLabel(airQuality));
  Serial.print("│ Light        : ");
  Serial.println(isDark ? "DARK           │" : "BRIGHT         │");
  Serial.print("│ Rain         : ");
  Serial.println(isRaining ? "RAINING        │" : "DRY            │");
  Serial.println("└─────────────────────────────┘");
  Serial.println();

  // ════════════════════════════════════
  //        OLED DISPLAY OUTPUT
  // ════════════════════════════════════
  static uint8_t page = 0;

  display.clearDisplay();
  display.setTextColor(WHITE);

  switch (page) {

    // ── Page 0: Temp & Humidity (DHT11) ──
    case 0:
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("-- DHT11 Sensor --");
      display.drawLine(0, 10, 128, 10, WHITE);

      display.setTextSize(2);
      display.setCursor(0, 16);
      display.print(tempDHT, 1);
      display.println(" C");

      display.setTextSize(1);
      display.setCursor(0, 38);
      display.print("Humidity : ");
      display.print(humidity, 1);
      display.println(" %");

      display.setCursor(0, 52);
      display.print("Feels: ");
      display.print(heatIndex(tempDHT, humidity), 1);
      display.println(" C");
      break;

    // ── Page 1: Pressure & Altitude (BMP280) ──
    case 1:
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("-- BMP280 Sensor --");
      display.drawLine(0, 10, 128, 10, WHITE);

      display.setCursor(0, 16);
      display.print("Pressure : ");
      display.print(pressure, 1);
      display.println(" hPa");

      display.setCursor(0, 30);
      display.print("Altitude : ");
      display.print(altitude, 1);
      display.println(" m");

      display.setCursor(0, 44);
      display.print("Temp(BMP): ");
      display.print(tempBMP, 1);
      display.println(" C");
      break;

    // ── Page 2: Air Quality (MQ-135) ──
    case 2:
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("-- Air Quality --");
      display.drawLine(0, 10, 128, 10, WHITE);

      display.setTextSize(3);
      display.setCursor(10, 18);
      display.println(airQuality);

      display.setTextSize(1);
      display.setCursor(0, 52);
      display.print("Status: ");
      display.println(aqLabel(airQuality));
      break;

    // ── Page 3: Light & Rain ──
    case 3:
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("-- Environment --");
      display.drawLine(0, 10, 128, 10, WHITE);

      display.setCursor(0, 18);
      display.print("Light  : ");
      display.println(isDark ? "DARK  " : "BRIGHT");

      display.setCursor(0, 32);
      display.print("Rain   : ");
      display.println(isRaining ? "RAINING" : "DRY    ");

      // Visual bar for air quality
      display.setCursor(0, 46);
      display.print("AQ Bar: [");
      int bars = map(airQuality, 0, 1023, 0, 10);
      for (int i = 0; i < 10; i++) {
        display.print(i < bars ? "|" : " ");
      }
      display.println("]");
      break;
  }

  display.display();
  page = (page + 1) % 4;    // rotate page every 3 seconds

  for (int i = 0; i < 30; i++) {
  server.handleClient();
  delay(100);
}
}

void handleReadADC() {

  float humidity = dht.readHumidity();
  float temp = dht.readTemperature();
  float pressure = bmp.readPressure() / 100.0F;
  int mq135 = analogRead(MQ135_PIN);
  int rain = !digitalRead(RAIN_PIN) ? 100 : 0;

  String json = "{";
  json += "\"Temperature\":" + String(temp,1) + ",";
  json += "\"Humidity\":" + String(humidity,1) + ",";
  json += "\"Pressuremb\":" + String(pressure,1) + ",";
  json += "\"MQ135\":" + String(mq135) + ",";
  json += "\"Rain\":" + String(rain);
  json += "}";

  server.send(200, "application/json", json);
}

// ─── Air Quality Label ───────────────────────────────────────
String aqLabel(int raw) {
  if (raw < 200)  return "GOOD     ";
  if (raw < 400)  return "MODERATE ";
  if (raw < 600)  return "UNHEALTHY";
  return               "HAZARDOUS";
}

// ─── Heat Index (Feels Like) ─────────────────────────────────
float heatIndex(float t, float h) {
  // Simplified Steadman formula
  return -8.78469475556
    + 1.61139411 * t
    + 2.3385248  * h
    - 0.14611605 * t * h
    - 0.01230809 * t * t
    - 0.01642482 * h * h
    + 0.00221173 * t * t * h
    + 0.00072546 * t * h * h
    - 0.00000358 * t * t * h * h;
}