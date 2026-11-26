#include <Arduino.h>
#include <HX711.h>
#include <DHTesp.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define HX_DT 16
#define HX_SCK 17
#define DHT 15

#define LED_GREEN 25
#define LED_YELLOW 26
#define LED_RED 27

#define BUZZER 23

#define OLED_SDA 21
#define OLED_SCL 22

//configs constantes

const float DAILY_CONSUMPTION = 0,3;
const float SCALE_FACTOR = 420.0;

const unsigned long READ_INTERVAL = 5000;

const float DAYS_WARNING = 5.0;
const float DAYS_CRITICAL = 2.0;

const float TEMP_WARNING = 30.0;
const float TEMP_CRITICAL = 40.0;

const float HUM_WARNING = 65.0;
const float HUM_CRITICAL = 80.0;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Hello, ESP32!");
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(10); // this speeds up the simulation
}
