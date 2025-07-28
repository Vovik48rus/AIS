#include <Arduino.h>
#include <Looper.h>
#include <ArduinoRS485.h>
#include <ArduinoModbus.h>

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

#include "Logger.h"
#include "CSMSModbus.h"
#include "Pump.h"
#include "Pot.h"
#include "HumiditySensorGroup.h"

/* Put your SSID & Password */
const char *ssid = "ESP32";        // Enter SSID here
const char *password = "12345678"; // Enter Password here

/* Put IP Address details */
IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer server(80);

// Настройка датчиков CSMS Modbus
CSMSModbus sensor1("CSMS-MOD1", &logger, 1, 0x00, 1000);
CSMSModbus sensor2("CSMS-MOD2", &logger, 2, 0x00, 1000);

// Группа для pot1
HumiditySensorGroup pot1Group("Pot1Group");

Pump pump1(18);

// Настройка pot1 с группой датчиков
Pot pot1("Pot1", &logger, &pump1, &pot1Group, 40, 3000, 1000, 2000);

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
