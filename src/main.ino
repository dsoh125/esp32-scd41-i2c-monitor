#include <Wire.h>
#include <SensirionI2cScd4x.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- OLED 설정 (128x32) ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

#define SDA_PIN 4
#define SCL_PIN 5

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
SensirionI2cScd4x scd4x;

unsigned long lastMsg = 0;
const long interval = 5000;

void drawScreen(uint16_t co2, float temperature, float humidity) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // 윗줄: CO2 숫자 크게 + ppm 작게
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print(co2);

  display.setTextSize(1);
  display.setCursor(60, 8);
  display.print("ppm");

  // 아랫줄: 온도/습도 한 줄
  display.setTextSize(1);
  display.setCursor(0, 22);
  display.print("T:");
  display.print(temperature, 1);
  display.print("C  H:");
  display.print(humidity, 1);
  display.print("%");

  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(SDA_PIN, SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed. Check wiring or I2C address!"));
    for (;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Initializing SCD41...");
  display.display();
  delay(1000);

  scd4x.begin(Wire, 0x62);

  uint16_t error;
  char errorMessage[256];

  error = scd4x.stopPeriodicMeasurement();
  if (error) {
    errorToString(error, errorMessage, 256);
    Serial.print("stopPeriodicMeasurement error: ");
    Serial.println(errorMessage);
  }

  error = scd4x.startPeriodicMeasurement();
  if (error) {
    errorToString(error, errorMessage, 256);
    Serial.print("startPeriodicMeasurement error: ");
    Serial.println(errorMessage);
  }
}

void loop() {
  unsigned long now = millis();
  if (now - lastMsg > interval) {
    lastMsg = now;

    uint16_t error;
    char errorMessage[256];
    bool isDataReady = false;

    error = scd4x.getDataReadyStatus(isDataReady);
    if (error) {
      errorToString(error, errorMessage, 256);
      Serial.print("getDataReadyStatus error: ");
      Serial.println(errorMessage);
      return;
    }

    if (isDataReady) {
      uint16_t co2 = 0;
      float temperature = 0.0f;
      float humidity = 0.0f;

      error = scd4x.readMeasurement(co2, temperature, humidity);
      if (error) {
        errorToString(error, errorMessage, 256);
        Serial.print("readMeasurement error: ");
        Serial.println(errorMessage);
      } else if (co2 == 0) {
        Serial.println("Invalid sample detected, skipping.");
      } else {
        Serial.print("Co2: ");
        Serial.print(co2);
        Serial.print("\tTemp: ");
        Serial.print(temperature);
        Serial.print("\tHum: ");
        Serial.println(humidity);

        drawScreen(co2, temperature, humidity);
      }
    }
  }
}
