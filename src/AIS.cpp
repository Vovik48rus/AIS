#include <Arduino.h>
#include <Looper.h>
#include <ArduinoRS485.h>
#include <ArduinoModbus.h>

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

#include <LcdMenu.h>
#include <MenuScreen.h>
#include <ItemCommand.h>
#include <ItemToggle.h>
#include <ItemSubMenu.h>
#include <ItemWidget.h>
#include <ItemRange.h>
#include <ItemValue.h>
#include <widget/WidgetList.h>
#include <widget/WidgetBool.h>
#include <display/LiquidCrystal_I2CAdapter.h>
#include <renderer/CharacterDisplayRenderer.h>
#include <input/KeyboardAdapter.h>
#include <SimpleRotary.h>
#include <input/SimpleRotaryAdapter.h>

#include "Logger.h"
#include "CSMSModbus.h"
#include "Pump.h"
#include "Pot.h"
#include "HumiditySensorGroup.h"
#include <Menu/CSMSModbusGetData.h>

#define LCD_ROWS 2
#define LCD_COLS 16
#define ENCODER_PIN_A 5
#define ENCODER_PIN_B 18
#define ENCODER_PIN_SW 19

// #define CREATE_SENSOR_MENU(name, sensorPtr)                                               \
//   CSMSModbusGetData name##GetData(sensorPtr);                                             \
//   CSMSModbusData* name##Data = name##GetData.getData();                                   \
//                                                                                           \
//   MENU_SCREEN(name##Screen, name##Items,                                                  \
//     ITEM_VALUE("Humidity", name##Data->humidityPtr, "%d"),                                \
//     ITEM_RANGE_REF<unsigned int>("Dry Soil", name##Data->drySoilValuePtr, 1, 0, 1023,     \
//       [](const Ref<unsigned int> value) {                                                 \
//         name##GetData.getSensor()->setDrySoilValue(value.value);                          \
//       }, "%d"),                                                                            \
//     ITEM_RANGE_REF<unsigned int>("Wet Soil", name##Data->wetSoilValuePtr, 1, 0, 1023,     \
//       [](const Ref<unsigned int> value) {                                                 \
//         name##GetData.getSensor()->setWetSoilValue(value.value);                          \
//       }, "%d"),                                                                            \
//     ITEM_COMMAND("Set Dry Soil", []() {                                                   \
//       if (name##GetData.getSensor()->calibrateDrySoilValue()) {                           \
//         logger.send(LevelLog::WARNING,                                                    \
//           (String("Dry Soil value set to: ") +                                            \
//            name##GetData.getSensor()->getDrySoilValue()).c_str());                        \
//       } else {                                                                             \
//         logger.send(LevelLog::ERROR, "Failed to set Dry Soil value due to invalid humidity."); \
//       }                                                                                   \
//     }),                                                                                   \
//     ITEM_COMMAND("Set Wet Soil", []() {                                                   \
//       if (name##GetData.getSensor()->calibrateWetSoilValue()) {                           \
//         logger.send(LevelLog::WARNING,                                                    \
//           (String("Wet Soil value set to: ") +                                            \
//            name##GetData.getSensor()->getWetSoilValue()).c_str());                        \
//       } else {                                                                             \
//         logger.send(LevelLog::ERROR, "Failed to set Wet Soil value due to invalid humidity."); \
//       }                                                                                   \
//     })                                                                                    \
//   )

#define CREATE_SENSOR_MENU(name, sensorPtr)                                                 \
  CSMSModbusGetData name##GetData(sensorPtr);                                               \
  CSMSModbusData* name##Data = name##GetData.getData();                                     \
                                                                                             \
  MENU_SCREEN(name##Screen, name##Items,                                                    \
    ITEM_VALUE("Humidity", name##Data->humidityPtr, "%d"),                                  \
    ITEM_VALUE("Row value", name##Data->medium, "%d"),                                       \
    ITEM_COMMAND("Update Soil", []() {                                                      \
      name##GetData.poll();                                                                 \
    }),                                                                                      \
    ITEM_RANGE_REF<unsigned int>("Dry Soil", name##Data->drySoilValuePtr, 1, 0, 1023,       \
      [](const Ref<unsigned int> value) {                                                   \
        Serial.println(value.value);                                                        \
        name##GetData.getSensor()->setDrySoilValue(value.value);                            \
        name##GetData.poll();                                                               \
      }, "%d"),                                                                              \
    ITEM_RANGE_REF<unsigned int>("Wet Soil", name##Data->wetSoilValuePtr, 1, 0, 1023,       \
      [](const Ref<unsigned int> value) {                                                   \
        name##GetData.getSensor()->setWetSoilValue(value.value);                            \
        name##GetData.poll();                                                               \
      }, "%d"),                                                                              \
    ITEM_COMMAND("Set Dry Soil", []() {                                                     \
      if (name##GetData.getSensor()->calibrateDrySoilValue()) {                             \
        logger.send(LevelLog::WARNING, (String("Dry Soil value set to: ") +                 \
          name##GetData.getSensor()->getDrySoilValue()).c_str());                           \
        name##GetData.poll();                                                               \
      } else {                                                                               \
        logger.send(LevelLog::ERROR, "Failed to set Dry Soil value due to invalid humidity."); \
      }                                                                                      \
    }),                                                                                      \
    ITEM_COMMAND("Set Wet Soil", []() {                                                     \
      if (name##GetData.getSensor()->calibrateWetSoilValue()) {                             \
        logger.send(LevelLog::WARNING, (String("Wet Soil value set to: ") +                 \
          name##GetData.getSensor()->getWetSoilValue()).c_str());                           \
        name##GetData.poll();                                                               \
      } else {                                                                               \
        logger.send(LevelLog::ERROR, "Failed to set Wet Soil value due to invalid humidity."); \
      }                                                                                      \
    })                                                                                       \
  )


/* Put your SSID & Password */
const char *ssid = "ESP32";        // Enter SSID here
const char *password = "12345678"; // Enter Password here

/* Put IP Address details */
IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer server(80);

// Настройка датчиков CSMS Modbus
CSMSModbus sensor1("CSMS-MOD1", &logger, 1, 0x00, 1000, 567, 225);
CSMSModbus sensor2("CSMS-MOD2", &logger, 2, 0x00, 1000, 567, 225);

// Группа для pot1
HumiditySensorGroup pot1Group("Pot1Group");

Pump pump1(18);

// Настройка pot1 с группой датчиков
Pot pot1("Pot1", &logger, &pump1, &pot1Group, 40, 3000, 1000, 2000);

// Initialize the main menu items
// clang-format off

// MENU_SCREEN(mainScreen, mainItems,
//     ITEM_BASIC("Start service"),
//     ITEM_BASIC("Connect to WiFi"),
//     ITEM_BASIC("Settings"),
//     ITEM_BASIC("Blink SOS"),
//     ITEM_BASIC("Blink random"));

// clang-format on

LiquidCrystal_I2C lcd(0x27, LCD_COLS, LCD_ROWS);
CharacterDisplayRenderer renderer(new LiquidCrystal_I2CAdapter(&lcd), LCD_COLS, LCD_ROWS);
LcdMenu menu(renderer);
KeyboardAdapter keyboard(&menu, &Serial);
SimpleRotary encoder(ENCODER_PIN_A, ENCODER_PIN_B, ENCODER_PIN_SW);
SimpleRotaryAdapter encoderA(&menu, &encoder);

// Счётчик итераций loop
unsigned long long counterLoop = 0;

// Таймер мониторинга памяти
LP_TIMER(500, []()
         {
  LevelLog level = (esp_get_free_heap_size() < 10000) ? LevelLog::WARNING : LevelLog::DEBUG;
  logger.send(level, (String("Свободная память (heap): ") + esp_get_free_heap_size()).c_str()); });

// Таймер мониторинга производительности
LP_TIMER(1000, []()
         {
  LevelLog level = (counterLoop < 100) ? LevelLog::WARNING : LevelLog::DEBUG;
  logger.send(level, (String("Количество Loop/Ms: ") + counterLoop).c_str());
  counterLoop = 0; });

LP_TIMER(1000, []() {

});

LP_TIMER(1000, []() {

});


// CSMSModbusGetData sensor1GetData(&sensor2);
// CSMSModbusData* sensor1Data = sensor1GetData.getData();

int a = 10;

CREATE_SENSOR_MENU(sensor1, &sensor1);
CREATE_SENSOR_MENU(sensor2, &sensor2);

MENU_SCREEN(
  CSMSsMenu, 
  CSMSsItems,
  ITEM_SUBMENU(sensor1.getName(), sensor1Screen),
  ITEM_SUBMENU(sensor2.getName(), sensor2Screen),  
);



// MENU_SCREEN(
//   sensor1Screen, 
//   sensor1Items,
//   ITEM_VALUE("Humidity", sensor1Data->humidityPtr, "%d"),
//   ITEM_VALUE("Row value", sensor1Data->medium, "%d"),
//   ITEM_COMMAND("Update Soil", [](){
//     sensor1GetData.poll();
//   }),
//   ITEM_RANGE_REF<unsigned int>("Dry Soil", sensor1Data->drySoilValuePtr, 1, 0, 1023, [](const Ref<unsigned int> value) {
//         Serial.println(value.value);
//         sensor1GetData.getSensor()->setDrySoilValue(value.value); 
//         sensor1GetData.poll();
//     }, "%d"),
//   ITEM_RANGE_REF<unsigned int>("Wet Soil", sensor1Data->wetSoilValuePtr, 1, 0, 1023, [](const Ref<unsigned int> value) {
//         sensor1GetData.getSensor()->setWetSoilValue(value.value); 
//         sensor1GetData.poll();
//     }, "%d"),
//   ITEM_COMMAND("Set Dry Soil", []() {
//   if(sensor1GetData.getSensor()->calibrateDrySoilValue())
//   {
//     logger.send(LevelLog::WARNING, (String("Dry Soil value set to: ") + sensor1GetData.getSensor()->getDrySoilValue()).c_str());
//     sensor1GetData.poll();
//   }
//   else
//     logger.send(LevelLog::ERROR, "Failed to set Dry Soil value due to invalid humidity.");
//   }),
//   ITEM_COMMAND("Set Wet Soil", []() {
//   if(sensor1GetData.getSensor()->calibrateWetSoilValue())
//     {
//       logger.send(LevelLog::WARNING, (String("Wet Soil value set to: ") + sensor1GetData.getSensor()->getWetSoilValue()).c_str());
//       sensor1GetData.poll();
//     }
//   else
//     logger.send(LevelLog::ERROR, "Failed to set Wet Soil value due to invalid humidity.");
//   }),
// );



void handleRoot();

void setup()
{
  logger.setLevelLog(LevelLog::WARNING);
  Serial.begin(115200);

  // Добавление датчиков в группу
  pot1Group.addSensor(&sensor1);
  pot1Group.addSensor(&sensor2);

  // Настройка Modbus RTU
  RS485.setPins(17, 33, 32);
  if (!ModbusRTUClient.begin(115200, SERIAL_8E2))
  {
    Serial.println("Не удалось запустить Modbus RTU Client!");
    while (1)
      ;
  }
  ModbusRTUClient.setTimeout(500);

  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);

  server.on("/", handleRoot);

  server.begin();

  renderer.begin();
  menu.setScreen(  CSMSsMenu);
}

void loop()
{
  Looper.loop();
  // delay(Looper.nextTimerLeft());
  // Serial.print(Looper.nextTimerLeft() * 1000);
  // Serial.print(", ");
  // Serial.print(millis());
  // Serial.print(", ");
  // Serial.print(pot1.left());
  // Serial.println();

  // if (Serial)
  // {
  //   Serial.flush();
  // }

  // esp_sleep_enable_timer_wakeup(Looper.nextTimerLeft() * 1000);
  // esp_light_sleep_start();
  counterLoop++;
  keyboard.observe();
  encoderA.observe();
  menu.poll(100);
  sensor1GetData.updateHumidity();
  sensor2GetData.updateHumidity();
}

void handleRoot()
{
  // Создаем JSON-объект
  StaticJsonDocument<200> jsonDoc;
  jsonDoc["temperature"] = 24.5;
  jsonDoc["humidity"] = 55;
  jsonDoc["status"] = "OK";

  // Сериализация в строку
  String jsonResponse;
  serializeJson(jsonDoc, jsonResponse);

  // Отправляем клиенту JSON
  server.send(200, "application/json", jsonResponse);
}
