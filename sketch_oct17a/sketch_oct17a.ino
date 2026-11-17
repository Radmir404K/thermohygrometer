#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_SHT31.h>
#include "HX711.h"

// --- Пины ---
#define I2C_DISPLAY_SDA 21
#define I2C_DISPLAY_SCL 15
#define I2C_SHT_SDA 18
#define I2C_SHT_SCL 19
#define HX711_DT 32
#define HX711_SCK 33
#define PUMP_PIN 25 // Пин управления насосом

// --- Настройки ---
float calibration_factor = 800.0; // Коэффициент вашего тензодатчика
const float EMPTY_TUBE_WEIGHT = 50.0; // Вес пустой пробирки в граммах (для защиты от сухого хода)
const float HUMIDITY_ON = 40.0;  
const float HUMIDITY_OFF = 45.0; 

TwoWire I2C_Sensor = TwoWire(1);
Adafruit_SSD1306 display(128, 64, &Wire, -1);
Adafruit_SHT31 sht30 = Adafruit_SHT31(&I2C_Sensor);
HX711 scale;

bool pumpState = false;

void setup() {
  Serial.begin(115200);
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW); // Насос выключен при старте

  // Инициализация дисплея
  Wire.begin(I2C_DISPLAY_SDA, I2C_DISPLAY_SCL); 
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { for(;;); }
  display.clearDisplay();
  display.setTextColor(WHITE);
  
  // Инициализация SHT30
  I2C_Sensor.begin(I2C_SHT_SDA, I2C_SHT_SCL); 
  if (!sht30.begin(0x44)) { for(;;); }

  // Инициализация весов
  scale.begin(HX711_DT, HX711_SCK);
  scale.set_scale(calibration_factor); 
  
  Serial.println("Система готова!");
}

void loop() {
  // 1. Читаем данные с датчиков
  float t = sht30.readTemperature(); 
  float h = sht30.readHumidity();    
  
  float currentWeight = 0;
  if (scale.is_ready()) {
    currentWeight = abs(scale.get_units(5)); 
  }

  // 2. Скрытая математика для защиты насоса (вычитаем вес пустой пробирки)
  float waterWeight = currentWeight - EMPTY_TUBE_WEIGHT;
  if (waterWeight < 0) waterWeight = 0;

  // 3. Логика управления насосом (с защитой от сухого хода)
  if (!pumpState && h < HUMIDITY_ON && waterWeight > 2.0) {
    pumpState = true;
    digitalWrite(PUMP_PIN, HIGH); 
  } 
  else if (pumpState && (h > HUMIDITY_OFF || waterWeight <= 1.0)) {
    pumpState = false;
    digitalWrite(PUMP_PIN, LOW); 
  }

  // 4. Вывод на экран (3 значения)
  if (!isnan(t) && !isnan(h)) {
    display.clearDisplay();
    display.setTextSize(2);
    
    // Строка 1: Температура
    display.setCursor(0, 0);
    display.print("T:"); 
    display.print(t, 1); 
    display.print("C");

    // Строка 2: Влажность
    display.setCursor(0, 22);
    display.print("H:"); 
    display.print(h, 1); 
    display.print("%");

    // Строка 3: Общий вес на весах (в граммах)
    display.setCursor(0, 44);
    display.print("W:"); 
    display.print(currentWeight, 0); 
    display.print("g");
    
    // Маленький индикатор работы насоса в правом нижнем углу
    if (pumpState) {
      display.setTextSize(1);
      display.setCursor(88, 56); // Позиция в углу
      display.print("PUMP ON");
    }
    
    display.display(); 
  }
  delay(1000); 
}
