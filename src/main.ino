#include <Arduino.h>
#include <HX711.h>
#include <DHTesp.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define HX_DT 16
#define HX_SCK 17

#define DHT_PIN 15

#define LED_GREEN 25
#define LED_YELLOW 26
#define LED_RED 27

#define BUZZER 23

#define OLED_SDA 21
#define OLED_SCL 22

const float DAILY_CONSUMPTION = 0.3;

const float SCALE_FACTOR = 420.0;

const unsigned long READ_INTERVAL = 5000;

const float DAYS_WARNING = 5.0;
const float DAYS_CRITICAL = 2.0;

const float TEMP_WARNING = 30.0;
const float TEMP_CRITICAL = 40.0;

const float HUM_WARNING = 65.0;
const float HUM_CRITICAL = 80.0;

const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
const int OLED_ADDRESS = 0x3C;

const int BUZZER_FREQUENCY = 1000;
const int BUZZER_DURATION = 500;

HX711 scale;
DHTesp dht;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

bool displayReady = false;

enum Status
{
  NORMAL,
  ATTENTION,
  CRITICAL
};

struct SensorData
{
  float weight;
  float temperature;
  float humidity;
  float autonomy;
};

const char *getStatusName(Status status)
{
  switch (status)
  {
  case NORMAL:
    return "NORMAL";
  case ATTENTION:
    return "ATTENTION";
  case CRITICAL:
    return "CRITICAL";
  default:
    return "UNKNOWN";
  }
}

bool readSensors(SensorData &data)
{
  if (!scale.wait_ready_timeout(1000))
  {
    Serial.println("[ERRO]  Sensor de peso (HX711) não encontrado!");
    return false;
  }

  data.weight = scale.get_units(1);

  if (!isfinite(data.weight))
  {
    Serial.println("[ERRO]  Leitura de peso inválida!");
    return false;
  }

  if (data.weight < 0)
  {
    Serial.println("[ERRO]  Leitura de peso negativa!");
    return false;
  }

  if (data.weight > 10000)
  {
    Serial.println("[ERRO]  Leitura de peso muito alta!");
    return false;
  }

  TempAndHumidity environment = dht.getTempAndHumidity();

  data.temperature = environment.temperature;
  data.humidity = environment.humidity;

  if (
      dht.getStatus() != DHTesp::ERROR_NONE ||
      !isfinite(data.temperature) ||
      !isfinite(data.humidity))
  {
    Serial.println("[ERRO]  Leitura de temperatura/umidade inválida!");
    return false;
  }

  if (data.humidity < 0 || data.humidity > 100)
  {
    Serial.println("[ERRO]  Leitura de umidade fora do intervalo válido!");
    return false;
  }

  if (data.temperature < -40 || data.temperature > 125)
  {
    Serial.println("[ERRO]  Leitura de temperatura fora do intervalo válido!");
    return false;
  }

  data.autonomy = data.weight / DAILY_CONSUMPTION;

  return true;
}

Status classify(const SensorData &data)
{
  if (data.autonomy <= DAYS_CRITICAL || data.temperature >= TEMP_CRITICAL || data.humidity >= HUM_CRITICAL)
  {
    return CRITICAL;
  }
  if (data.autonomy <= DAYS_WARNING || data.temperature >= TEMP_WARNING || data.humidity >= HUM_WARNING)
  {
    return ATTENTION;
  }
  return NORMAL;
}

void updateActuators(Status status)
{
  digitalWrite(LED_GREEN, status == NORMAL ? HIGH : LOW);
  digitalWrite(LED_YELLOW, status == ATTENTION ? HIGH : LOW);
  digitalWrite(LED_RED, status == CRITICAL ? HIGH : LOW);

  if (status == CRITICAL)
  {
    tone(BUZZER, BUZZER_FREQUENCY, BUZZER_DURATION);
  }
  else
  {
    noTone(BUZZER);
  }
}

void printReasons(const SensorData &data)
{
  if (data.autonomy <= DAYS_CRITICAL)
  {
    Serial.println("[CRITICO]  Estoque crítico! Autonomia: " + String(data.autonomy, 2) + " dias");
  }
  if (data.autonomy <= DAYS_WARNING && data.autonomy > DAYS_CRITICAL)
  {
    Serial.println("[ATENCAO]  Estoque baixo! Autonomia: " + String(data.autonomy, 2) + " dias");
  }

  if (data.temperature >= TEMP_CRITICAL)
  {
    Serial.println("[CRITICO]  Temperatura crítica! Temperatura: " + String(data.temperature, 2) + " °C");
  }
  if (data.temperature >= TEMP_WARNING && data.temperature < TEMP_CRITICAL)
  {
    Serial.println("[ATENCAO]  Temperatura alta! Temperatura: " + String(data.temperature, 2) + " °C");
  }

  if (data.humidity >= HUM_CRITICAL)
  {
    Serial.println("[CRITICO]  Umidade crítica! Umidade: " + String(data.humidity, 2) + " %");
  }
  if (data.humidity >= HUM_WARNING && data.humidity < HUM_CRITICAL)
  {
    Serial.println("[ATENCAO]  Umidade alta! Umidade: " + String(data.humidity, 2) + " %");
  }
}

void updateDisplay(Status status, const SensorData &data)
{
  if (!displayReady)
  {
    return;
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);

  display.println("SMARTPANTRY");
  display.println("---------------------");

  display.printf("Status: ", String(getStatusName(status)));

  display.printf("Peso: %.2f g\n", data.weight);

  display.printf(
      "Temp: %.1f C\n",
      data.temperature);

  display.printf(
      "Umid: %.1f %%\n",
      data.humidity);

  display.printf(
      "Estoque: %.1f dias\n",
      data.autonomy);

  display.display();
}

void printSerial(
    Status status,
    const SensorData &data)
{

  Serial.println("========================");

  Serial.printf(
      "Status: %s\n",
      getStatusName(status));

  Serial.printf(
      "Peso: %.2f kg\n",
      data.weight);

  Serial.printf(
      "Temperatura: %.1f C\n",
      data.temperature);

  Serial.printf(
      "Umidade: %.1f %%\n",
      data.humidity);

  Serial.printf(
      "Autonomia: %.2f dias\n",
      data.autonomy);

  if (status != NORMAL)
  {
    printReasons(data);
  }

  Serial.println("========================");
}

void handleSensorError()
{

  updateActuators(CRITICAL);

  Serial.println("========================");
  Serial.println("STATUS: CRITICO");
  Serial.println("Falha na leitura dos sensores");
  Serial.println("Verifique as conexoes");
  Serial.println("========================");

  if (displayReady)
  {

    display.clearDisplay();
    display.setCursor(0, 0);

    display.println("SMARTPANTRY");
    display.println("---------------------");
    display.println("STATUS: CRITICO");
    display.println("Falha nos sensores");
    display.println("Verifique o circuito");

    display.display();
  }
}

void setup()
{
  Serial.begin(115200);

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(BUZZER, LOW);

  scale.begin(HX_DT, HX_SCK);
  scale.set_scale(SCALE_FACTOR);
  scale.set_offset(0);

  dht.setup(DHT_PIN, DHTesp::DHT22);

  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setTimeOut(50);

  Wire.beginTransmission(OLED_ADDRESS);

  if (Wire.endTransmission() == 0)
  {
    displayReady = display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS);
  }

  if (displayReady)
  {

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);

    display.println("SMARTPANTRY");
    display.println("Inicializando! ...");

    display.display();
  }

  if (!displayReady)
  {
    Serial.println("[ERRO]  Display OLED não encontrado!");
  };

  Serial.println("========================");
  Serial.println("SMARTPANTRY");
  Serial.println("Sistema inicializado");
  Serial.println("========================");

  delay(2000);
}

void loop()
{

  SensorData data;

  if (!readSensors(data))
  {

    handleSensorError();

    delay(READ_INTERVAL);

    return;
  }

  Status status = classify(data);

  updateActuators(status);

  updateDisplay(status, data);

  printSerial(status, data);

  delay(READ_INTERVAL);
};