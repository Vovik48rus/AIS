#include <limits>

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
#include <Menu/PotGetData.h>
#include <Menu/PumpGetData.h>
#include <CSMSModbusTest.h>

using namespace std;

#define LCD_ROWS 2
#define LCD_COLS 16
#define ENCODER_PIN_A 33  // clk
#define ENCODER_PIN_B 32  // dt
#define ENCODER_PIN_SW 35 // sw

#define CREATE_SENSOR_MENU(name, sensorPtr)                                \
  CSMSModbusGetData name##GetData(sensorPtr);                              \
  CSMSModbusData *name##Data = name##GetData.getData();                    \
                                                                           \
  MENU_SCREEN(name##Screen, name##Items,                                   \
              ITEM_VALUE("Humidity", name##Data->humidityPtr, "%d"),       \
              ITEM_VALUE("Row value", name##Data->medium, "%d"),           \
              ITEM_COMMAND("Update Soil", []() { name##GetData.poll(); }), \
              ITEM_RANGE_REF<unsigned int>("Dry Soil", name##Data->drySoilValuePtr, 1, 0, 1023, [](const Ref<unsigned int> value) {                                                   \
        name##GetData.getSensor()->setDrySoilValue(value.value);                            \
        name##GetData.poll(); }, "%d"), ITEM_RANGE_REF<unsigned int>("Wet Soil", name##Data->wetSoilValuePtr, 1, 0, 1023, [](const Ref<unsigned int> value) {                                                   \
        name##GetData.getSensor()->setWetSoilValue(value.value);                            \
        name##GetData.poll(); }, "%d"), ITEM_COMMAND("Set Dry Soil", []() {                                                     \
      if (name##GetData.getSensor()->calibrateDrySoilValue()) {                             \
        logger.send(LevelLog::WARNING, (String("Dry Soil value set to: ") +                 \
          name##GetData.getSensor()->getDrySoilValue()).c_str());                           \
        name##GetData.poll();                                                               \
      } else {                                                                               \
        logger.send(LevelLog::ERROR, "Failed to set Dry Soil value due to invalid humidity."); \
      } }), ITEM_COMMAND("Set Wet Soil", []() {                                                     \
      if (name##GetData.getSensor()->calibrateWetSoilValue()) {                             \
        logger.send(LevelLog::WARNING, (String("Wet Soil value set to: ") +                 \
          name##GetData.getSensor()->getWetSoilValue()).c_str());                           \
        name##GetData.poll();                                                               \
      } else {                                                                               \
        logger.send(LevelLog::ERROR, "Failed to set Wet Soil value due to invalid humidity."); \
      } }))

#define CREATE_POT_MENU(name, potPtr, loggerPtr, pumpScreen, ...)                        \
  PotGetData name##GetData(potPtr, loggerPtr);                               \
  PotData* name##Data = name##GetData.getData();                             \
                                                                             \
  MENU_SCREEN(                                                               \
      name##CSMSMenu,                                                        \
      name##CSMSItems,                                                       \
      __VA_ARGS__ );                                                         \
                                                                             \
  MENU_SCREEN(                                                               \
      name##PeriodsMenu,                                                     \
      name##PeriodsItems,                                                    \
      ITEM_COMMAND("Update Periods", []() { name##GetData.poll(); }),        \
      ITEM_RANGE_REF<int>("Survey", name##Data->surveyTime, 50, 0,           \
          numeric_limits<int>::max(), [](const Ref<int> value) {             \
            name##GetData.getPot()->setSurveyTime(value.value);              \
            name##GetData.poll();                                            \
          }, "%d"),                                                          \
      ITEM_RANGE_REF<int>("Watering", name##Data->wateringTime, 50, 0,       \
          numeric_limits<int>::max(), [](const Ref<int> value) {             \
            name##GetData.getPot()->setWateringTime(value.value);            \
            name##GetData.poll();                                            \
          }, "%d"),                                                          \
      ITEM_RANGE_REF<int>("Absorption", name##Data->absorptionTime, 50, 0,   \
          numeric_limits<int>::max(), [](const Ref<int> value) {             \
            name##GetData.getPot()->setAbsorptionTime(value.value);          \
            name##GetData.poll();                                            \
          }, "%d") );                                                        \
                                                                             \
  MENU_SCREEN(                                                               \
      name##Menu,                                                            \
      name##Items,                                                           \
      ITEM_VALUE("Period", name##Data->stateStr, "%s"),                      \
      ITEM_VALUE("Humidity", name##Data->lastHumidity, "%d%%"),              \
      ITEM_RANGE_REF<int>("Threshold", name##Data->threshold, 1, 0, 100,     \
          [](const Ref<int> value) {                                         \
            name##GetData.getPot()->setThreshold(value.value);               \
            name##GetData.poll();                                            \
          }, "%d%%"),                                                        \
      ITEM_SUBMENU("Periods", name##PeriodsMenu),                            \
      ITEM_SUBMENU("Sensors", name##CSMSMenu),                               \
      ITEM_SUBMENU("Pump", pumpScreen) );                                    \
                                                                             \
  LP_TIMER(500, [](){                                                        \
    name##GetData.updateHumidity();                                          \
    name##GetData.updateState();                                             \
  });

  #define CREATE_PUMP_MENU(name, pumpPtr, loggerPtr)                       \
  PumpGetData name##GetData(pumpPtr, loggerPtr);                           \
  PumpData* name##Data = name##GetData.getData();                          \
                                                                           \
  MENU_SCREEN(                                                             \
      name##Menu,                                                          \
      name##Items,                                                         \
      ITEM_VALUE("State", name##Data->stateStr, "%s"),                     \
      ITEM_COMMAND("Turn ON", []() {                                       \
        name##GetData.getPump()->on();                                     \
        name##GetData.updateState();                                       \
      }),                                                                  \
      ITEM_COMMAND("Turn OFF", []() {                                      \
        name##GetData.getPump()->off();                                    \
        name##GetData.updateState();                                       \
      }),                                                                  \
      ITEM_COMMAND("Update State", []() {                                  \
        name##GetData.updateState();                                       \
      })                                                                   \
  );                                                                        \
                                                                           \
  LP_TIMER(500, [](){                                                      \
    name##GetData.updateState();                                           \
  });


extern Logger logger;

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
CSMSModbus sensor2("CSMS-MOD2", &logger, 3, 0x00, 1000, 567, 225);
CSMSModbus sensor3("CSMS-MOD3", &logger, 2, 0x00, 1000, 567, 225);
CSMSModbus sensor4("CSMS-MOD4", &logger, 4, 0x00, 1000, 567, 225);

// Группа для pot1
HumiditySensorGroup pot1Group("Pot1Group");
HumiditySensorGroup pot2Group("Pot2Group");

Pump pump1(19);
Pump pump2(23);

// Настройка pot1 с группой датчиков
Pot pot1("Pot1", &logger, &pump1, &pot1Group, 40, 3000, 1000, 2000);
Pot pot2("Pot2", &logger, &pump2, &pot2Group, 40, 3000, 1000, 2000);

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

// CSMSModbusGetData sensor1GetData(&sensor2);
// CSMSModbusData* sensor1Data = sensor1GetData.getData();

CREATE_SENSOR_MENU(sensor1, &sensor1);
CREATE_SENSOR_MENU(sensor2, &sensor2);

CREATE_PUMP_MENU(pump1, &pump1, &logger);
CREATE_PUMP_MENU(pump2, &pump2, &logger);

CREATE_POT_MENU(
    pot1,
    &pot1,
    &logger,
    pump1Menu,
    ITEM_SUBMENU(sensor1.getName(), sensor1Screen),
    ITEM_SUBMENU(sensor2.getName(), sensor2Screen)
);

CREATE_SENSOR_MENU(sensor3, &sensor3);
CREATE_SENSOR_MENU(sensor4, &sensor4);

CREATE_POT_MENU(
    pot2,
    &pot2,
    &logger,
    pump2Menu,
    ITEM_SUBMENU(sensor3.getName(), sensor3Screen),
    ITEM_SUBMENU(sensor4.getName(), sensor4Screen)
);

MENU_SCREEN(
    PumpsMenu,
    PumpsItems,
    ITEM_SUBMENU("Pump1", pump1Menu),
    ITEM_SUBMENU("Pump2", pump2Menu)
);

MENU_SCREEN(
    CSMSsMenu,
    CSMSsItems,
    ITEM_SUBMENU(sensor1.getName(), sensor1Screen),
    ITEM_SUBMENU(sensor2.getName(), sensor2Screen),
    ITEM_SUBMENU(sensor3.getName(), sensor3Screen),
    ITEM_SUBMENU(sensor4.getName(), sensor4Screen));

MENU_SCREEN(
    PotsMenu,
    PotsItems,
    ITEM_SUBMENU(pot1.getNameCStr(), pot1Menu),
    ITEM_SUBMENU(pot2.getNameCStr(), pot2Menu),
);

MENU_SCREEN(
    RootMenu,
    RootItems,
    ITEM_SUBMENU("CSMS Sensors", CSMSsMenu),
    ITEM_SUBMENU("Pots", PotsMenu),
    ITEM_SUBMENU("Pumps", PumpsMenu),
);

// MENU_SCREEN();


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

// PotGetData pot1GetData(&pot1, &logger);
// PotData *pot1Data = pot1GetData.getData();

// MENU_SCREEN(
//   Pot1CSMSMenu,
//   Pot1CSMSItems,
//   ITEM_SUBMENU(sensor1.getName(), sensor1Screen),
//   ITEM_SUBMENU(sensor2.getName(), sensor2Screen),
// );

// MENU_SCREEN(
//     Pot1PeriodsMenu,
//     Pot1PeriodsItems,
//     ITEM_COMMAND("Update Periods", []()
//                  { pot1GetData.poll(); }),
//     ITEM_RANGE_REF<int>("Survey", pot1Data->surveyTime, 50, 0, numeric_limits<int>::max(), [](const Ref<int> value)
//                         {
//         pot1GetData.getPot()->setSurveyTime(value.value);
//         pot1GetData.poll(); }, "%d"),
//     ITEM_RANGE_REF<int>("Watering", pot1Data->wateringTime, 50, 0, numeric_limits<int>::max(), [](const Ref<int> value)
//                         {
//         pot1GetData.getPot()->setWateringTime(value.value);
//         pot1GetData.poll(); }, "%d"),
//     ITEM_RANGE_REF<int>("Absorption", pot1Data->absorptionTime, 50, 0, numeric_limits<int>::max(), [](const Ref<int> value)
//                         {
//         pot1GetData.getPot()->setAbsorptionTime(value.value);
//         pot1GetData.poll(); }, "%d"), );

// MENU_SCREEN(
//     Pot1Menu,
//     Pot1Items,
//     ITEM_VALUE("Period", pot1Data->stateStr, "%s"),
//     ITEM_RANGE_REF<int>("Threshold", pot1Data->threshold, 1, 0, 100, [](const Ref<int> value)
//                         {
//         pot1GetData.getPot()->setThreshold(value.value); 
//         pot1GetData.poll(); }, "%d%%"),
//     ITEM_SUBMENU("Periods", Pot1PeriodsMenu),
//     ITEM_SUBMENU("Sensors", Pot1CSMSMenu));

// LP_TIMER(500, [](){
//   pot1GetData.updateHumidity();
//   pot1GetData.updateState();
// });

// MENU_SCREEN(
//   PotsMenu,
//   PotsItems,

// );

LP_TIMER(500, []()
         {
  menu.poll(500);
  logger.send(LevelLog::DEBUG, (String("Menu polled")).c_str()); });

LP_TIMER(500, []()
         {
  sensor1GetData.updateHumidity();
});

LP_TIMER(500, []()
         {
  sensor2GetData.updateHumidity();
});

LP_TIMER(10, []()
         {
           keyboard.observe();
           // encoderA.observe();
           // logger.send(LevelLog::DEBUG, (String("Keyboard and Encoder polled")).c_str());
         });

// LP_TIMER(500, []()
//          { Serial.println(pot1Data->stateStr); });

TimerHandle_t xTimer;

void updateEncoderA(TimerHandle_t xTimer)
{
  encoderA.observe();
}

void handleRoot();

void setup()
{
  logger.setLevelLog(LevelLog::WARNING);
  Serial.begin(115200);

  // Добавление датчиков в группу
  pot1Group.addSensor(&sensor1);
  pot1Group.addSensor(&sensor2);

  pot2Group.addSensor(&sensor3);
  pot2Group.addSensor(&sensor4);

  xTimer = xTimerCreate("MyTimer", pdMS_TO_TICKS(10), pdTRUE, (void *)0, updateEncoderA);
  xTimerStart(xTimer, 0);

  // Настройка Modbus RTU
  // RS485.setPins(17, 33, 32);
  if (!ModbusRTUClient.begin(9600, SERIAL_8N2))
  {
    Serial.println("Не удалось запустить Modbus RTU Client!");
    while (1)
      ;
  }
  ModbusRTUClient.setTimeout(300);

  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);

  server.on("/", handleRoot);

  server.begin();

  renderer.begin();
  menu.setScreen(RootMenu);
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
