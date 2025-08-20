#ifndef CSMS_MODBUS_H
#define CSMS_MODBUS_H

#include <Arduino.h>
#include <ArduinoRS485.h>
#include <ArduinoModbus.h>

#include "ICSMS.h"
#include "Logger.h"

enum CSMSModbusState
{
  NORMAL_SURVEY,
  REPEAT_SURVEY
};

class CSMSModbus : public ICSMS
{
private:
  uint8_t slaveId;
  uint16_t registerAddress;
  const char *name;
  uint32_t lastValueRead = 0;
  int humidity = -1;
  int medium = 0;
  unsigned int drySoilValue;
  unsigned int wetSoilValue;
  CSMSModbusState myCSMSModbusState;
  uint32_t normalInterval;
  uint32_t repeatInterval;

  Logger *logger;

  int failCount = 0;                    // количество последовательных неудач
  static constexpr int MAX_FAILS = 5;   // максимум последовательных неудач до сброса humidity
  static constexpr int MAX_RETRIES = 1; // количество попыток при одном опросе

public:
  CSMSModbus(const char *name, Logger *logger, uint8_t slaveId, uint16_t registerAddress, uint32_t normalInterval,
             unsigned int drySoilValue = 700, unsigned int wetSoilValue = 275, uint32_t repeatInterval = 500)
      : ICSMS(name, normalInterval),
        slaveId(slaveId),
        registerAddress(registerAddress),
        name(name),
        logger(logger),
        drySoilValue(drySoilValue),
        wetSoilValue(wetSoilValue),
        myCSMSModbusState(NORMAL_SURVEY),
        normalInterval(normalInterval),
        repeatInterval(repeatInterval)

  {
    logger->send(LevelLog::INFO, (String("Modbus CSMS sensor initialized: ") + name).c_str());
  }

  void exec() override
  {
    int attempt = 0;

    while (attempt < MAX_RETRIES)
    {
      // logger->flush();
      Serial2.flush();
      delay(100);
      medium = readInputRegister();
      Serial2.flush();
      if (medium != -1)
        break; // успех

      attempt++;
      logger->send(LevelLog::WARNING, (String("Attempt #") + attempt + " failed for [" + name + "], error: " + ModbusRTUClient.lastError()).c_str());
      // delay(100);
    }

    if (medium == -1)
    {
      failCount++;
      logger->send(LevelLog::WARNING, (String("All retries failed for [") + name + "], fail count = " + failCount).c_str());

      if (failCount >= MAX_FAILS)
      {
        humidity = -1;
        logger->send(LevelLog::ERROR, (String("Max fail count reached for ") + name + ", setting humidity = -1").c_str());
        failCount = 0;
      }

      this->changeCSMSModbusState(CSMSModbusState::REPEAT_SURVEY);

      return;
    }

    if (!mediumIsValid())
    {
      logger->send(LevelLog::ERROR, (String("Invalid medium value for ") + name + ": " + medium).c_str());
      this->changeCSMSModbusState(CSMSModbusState::REPEAT_SURVEY);
      return;
    }
    this->changeCSMSModbusState(CSMSModbusState::NORMAL_SURVEY);

    failCount = 0;
    lastValueRead = medium;
    humidity = ::map(medium, drySoilValue, wetSoilValue, 0, 100);
    logger->send(LevelLog::WARNING, (String("Sensor [") + name + "] raw: " + medium + ", humidity: " + humidity + "%").c_str());
  }

  bool humidityIsValid() const override
  {
    return humidity >= 0 && humidity <= 100;
  }

  bool mediumIsValid() const
  {
    return 0 <= medium && medium <= 1023;
  }

  int getHumidity() const override
  {
    return humidity;
  }

  const char *getName() const override
  {
    return name;
  }

  unsigned int getDrySoilValue() const override
  {
    return drySoilValue;
  }

  bool setDrySoilValue(unsigned int value) override
  {
    if (value < 1024 && value != wetSoilValue)
    {
      drySoilValue = value;
      return true;
    }
    logger->send(LevelLog::WARNING, (String("Invalid drySoilValue set attempt on ") + name).c_str());
    return false;
  }

  unsigned int getWetSoilValue() const override
  {
    return wetSoilValue;
  }

  bool setWetSoilValue(unsigned int value) override
  {
    if (value < 1024 && value != drySoilValue)
    {
      wetSoilValue = value;
      return true;
    }
    logger->send(LevelLog::WARNING, (String("Invalid wetSoilValue set attempt on ") + name).c_str());
    return false;
  }

  bool calibrateDrySoilValue()
  {
    if (mediumIsValid())
    {
      bool flag = setDrySoilValue(getMedium());
      if (flag)
        logger->send(LevelLog::WARNING, (String("Калибровка сухой почвы датчика #") + name + String(", новое значение: ") + getMedium()).c_str());
      else
        logger->send(LevelLog::ERROR, (String("Ошибка калибровки сухой почвы датчика #") + name).c_str());
      return flag;
    }
    else
    {
      logger->send(LevelLog::ERROR, (String("Ошибка калибровки сухой почвы датчика #") + name + String("значение не валидно ") + getMedium()).c_str());
      return false;
    }
  }

  bool calibrateWetSoilValue()
  {
    if (mediumIsValid())
    {
      bool flag = setWetSoilValue(getMedium());
      if (flag)
        logger->send(LevelLog::WARNING, (String("Калибровка влажной почвы датчика #") + name + String(", новое значение: ") + getMedium()).c_str());
      else
        logger->send(LevelLog::ERROR, (String("Ошибка калибровки влажной почвы датчика #") + name).c_str());
      return flag;
    }
    else
    {
      logger->send(LevelLog::ERROR, (String("Ошибка калибровки влажной почвы датчика #") + name + String("значение не валидно ") + getMedium()).c_str());
      return false;
    }
  }

  ~CSMSModbus() {}

  int getMedium() override
  {
    // Возвращаем среднее значение между сухой и влажной почвой
    return medium;
  }

  void changeCSMSModbusState(CSMSModbusState myCSMSModbusState)
  {
    if (this->myCSMSModbusState != myCSMSModbusState)
    {
      logger->send(LevelLog::WARNING, (String("Changing CSMSModbus state from ") + (this->myCSMSModbusState == CSMSModbusState::NORMAL_SURVEY ? "NORMAL_SURVEY" : "REPEAT_SURVEY") + " to " + (myCSMSModbusState == CSMSModbusState::NORMAL_SURVEY ? "NORMAL_SURVEY" : "REPEAT_SURVEY")).c_str());
      switch (myCSMSModbusState)
      {
      case CSMSModbusState::NORMAL_SURVEY:
        this->restart(normalInterval);
        break;

      case CSMSModbusState::REPEAT_SURVEY:
        this->restart(repeatInterval);
        break;

      default:
        break;
      }
      this->myCSMSModbusState = myCSMSModbusState;
    }
    else
    {
      logger->send(LevelLog::WARNING, (String("CSMSModbus state remains unchanged: ") + (this->myCSMSModbusState == CSMSModbusState::NORMAL_SURVEY ? "NORMAL_SURVEY" : "REPEAT_SURVEY")).c_str());
    }
  }

  protected:
  virtual long readInputRegister() const
  {
    return ModbusRTUClient.inputRegisterRead(slaveId, registerAddress);
  }

};

#endif // CSMS_MODBUS_H
