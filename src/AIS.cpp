#include <Arduino.h>
#include <Looper.h>
// #include <WiFi.h>
// #include <WebServer.h>
#include <ArduinoJson.h>
#include "Logger.h"
#include "CSMS.h"
#include "Pump.h"
#include "Pot.h"

/* Put your SSID & Password */
const char* ssid = "ESP32";  // Enter SSID here
const char* password = "12345678";  //Enter Password here

/* Put IP Address details */
// IPAddress local_ip(192,168,1,1);
// IPAddress gateway(192,168,1,1);
// IPAddress subnet(255,255,255,0);

// WebServer server(80);

CSMS CSMS1(32, &logger, "CSMS1", 100, 2800, 1050);
CSMS CSMS2(33, &logger, "CSMS2", 100, 2800, 1100);
CSMS CSMS3(34, &logger, "CSMS3", 100, 2800, 1100);
CSMS CSMS4(35, &logger, "CSMS4", 100, 2800, 1100);

Pump pump1(18);
Pump pump2(5);
// Pump pump3(17);
// Pump pump4(16);

Pot pot1("Pot1", &logger, &pump1, &CSMS1, 40, 3000, 1000, 2000);

unsigned long long counterLoop = 0;

LP_TIMER(500, []() 
{
  LevelLog messagelevel;
  if (esp_get_free_heap_size() < 10000) messagelevel = LevelLog::WARNING;
  else messagelevel = LevelLog::DEBUG;

  logger.send(messagelevel, (string("Свободная память (heap): ") + to_string(esp_get_free_heap_size())).c_str());
});

LP_TIMER(1000, []() 
{
  LevelLog messagelevel;
  if (counterLoop < 100) messagelevel = LevelLog::WARNING;
  else messagelevel = LevelLog::DEBUG;

  logger.send(messagelevel, (string("Количество Loop/Ms: ") + to_string(counterLoop)).c_str());
  counterLoop = 0;
});

void handleRoot();

void setup() 
{
  logger.setLevelLog(LevelLog::WARNING);
  Serial.begin(115200);

  // WiFi.softAP(ssid, password);
  // WiFi.softAPConfig(local_ip, gateway, subnet);
  // delay(100);
  
  // server.on("/", handleRoot);
  
  // server.begin();
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
  if (Serial)
  {
    Serial.flush();
  }
  // esp_sleep_enable_timer_wakeup(Looper.nextTimerLeft() * 1000);
  // esp_light_sleep_start();
  counterLoop += 1;
}

// void handleRoot() {
//   // Создаем JSON-объект
//   StaticJsonDocument<200> jsonDoc;
//   jsonDoc["temperature"] = 24.5;
//   jsonDoc["humidity"] = 55;
//   jsonDoc["status"] = "OK";

//   // Сериализация в строку
//   String jsonResponse;
//   serializeJson(jsonDoc, jsonResponse);

//   // Отправляем клиенту JSON
//   server.send(200, "application/json", jsonResponse);
// }
