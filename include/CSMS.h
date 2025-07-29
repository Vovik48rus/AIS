#ifndef CSMS_H
#define CSMS_H

#include "Logger.h"
#include <Looper.h>
#include "ICSMS.h"

using namespace std;

class CSMS : public ICSMS
{
  unsigned int pin;
  int arr[10] = {0};
  int counter;
  int medium;
  int humidity;
  unsigned int drySoilValue;
  unsigned int wetSoilValue;
  const char *name;

  Logger *logger;

public:
  CSMS(int pin, Logger *logger, const char *name, uint32_t intervalMs, unsigned int drySoilValue = 2800, unsigned int wetSoilValue = 1100) : ICSMS(name, intervalMs), pin(pin), logger(logger), name(name), drySoilValue(drySoilValue), wetSoilValue(wetSoilValue)
  {
    counter = 0;
    medium = 0;
    logger->send(LevelLog::INFO, (string("Подключение: #") + name).c_str());
  }

  void exec() override
  {
    measure();
    if (counter == 10)
    {
      calcHumidity();
    }
  }

  bool humidityIsValid() const override
  {
    if (0 <= this->humidity && this->humidity <= 100)
    {
      return true;
    }
    logger->send(LevelLog::WARNING, (string("Среднее значение влажности почвы датчика #") + name + string(" не является правдивым: ") + to_string(this->humidity) + string("%")).c_str());
    return false;
  }

  int getHumidity() const override
  {
    return this->humidity;
  }

  // bool mediumIsValid()
  // {
  //   if (wetSoilValue <= this->medium &&  this->medium <= drySoilValue)
  //   {
  //     return true;
  //   }
  //     logger->send(LevelLog::WARNING, (string("Среднее значение влажности почвы датчика #") + name + string(" не является правдивым: ") + to_string(this->medium) + string("")).c_str());
  //   return false;
  // }

  // int getMedium()
  // {
  //   return this->medium;
  // }

  const char *getName() const override
  {
    return this->name;
  }

  unsigned int getDrySoilValue() const override
  {
    return drySoilValue;
  }

  bool setDrySoilValue(unsigned int drySoilValue) override
  {
    if (drySoilValue < 4096)
    {
      this->drySoilValue = drySoilValue;
      return true;
    }
    logger->send(LevelLog::WARNING, (string("Неудачное изменение drySoilValue: #") + name + string(", drySoilValue которое пытались установить: ") + to_string(drySoilValue) + string(" > 4096")).c_str());
    return false;
  }

  unsigned int getWetSoilValue() const override
  {
    return wetSoilValue;
  }

  bool setWetSoilValue(unsigned int wetSoilValue) override
  {
    if (wetSoilValue < 4096)
    {
      this->wetSoilValue = wetSoilValue;
      return true;
    }
    logger->send(LevelLog::WARNING, (string("Неудачное изменение wetSoilValue: #") + name + string(", wetSoilValue которое пытались установить: ") + to_string(wetSoilValue) + string(" > 4096")).c_str());
    return false;
  }

  bool calibrateDrySoilValue()
  {
    if (humidityIsValid())
    {
      bool flag = setDrySoilValue(getMedium());
      if (flag)
        logger->send(LevelLog::WARNING, (string("Калибровка сухой почвы датчика #") + name + string(", новое значение: ") + to_string(getMedium())).c_str());
      else
        logger->send(LevelLog::ERROR, (string("Ошибка калибровки сухой почвы датчика #") + name).c_str());
      return flag;
    }
    else
    {
      logger->send(LevelLog::ERROR, (string("Ошибка калибровки сухой почвы датчика #") + name + string("значение не валидно ") + to_string(getMedium())).c_str());
      return false;
    }
  }

  bool calibrateWetSoilValue()
  {
    if (humidityIsValid())
    {
      bool flag = setWetSoilValue(getMedium());
      if (flag)
        logger->send(LevelLog::WARNING, (string("Калибровка влажной почвы датчика #") + name + string(", новое значение: ") + to_string(getMedium())).c_str());
      else
        logger->send(LevelLog::ERROR, (string("Ошибка калибровки влажной почвы датчика #") + name).c_str());
      return flag;
    }
    else
    {
      logger->send(LevelLog::ERROR, (string("Ошибка калибровки влажной почвы датчика #") + name + string("значение не валидно ") + to_string(getMedium())).c_str());
      return false;
    }
  }

  int getMedium()
  {
    return medium;
  }

  ~CSMS() {}

private:
  void measure()
  {
    logger->send(LevelLog::INFO, (string("Pin:") + to_string(pin) + string(", ") + to_string(analogRead(pin))).c_str());
    arr[counter] = analogRead(pin);
    logger->send(LevelLog::INFO, (
                                     string("Измерение данных: #") + name + string(", pin:") + to_string(pin) + string(", ") + string(", сырое значение: ") + to_string(arr[counter]))
                                     .c_str());
    counter++;
  }

  int calcMedium()
  {
    unsigned int summ = 0;
    for (int i = 0; i < 10; i++)
    {
      summ += arr[i];
    }
    medium = summ / 10;
    counter = 0;
    return medium;
  }

  int calcHumidity()
  {
    this->humidity = map(calcMedium(), drySoilValue, wetSoilValue, 0, 100);
    return this->humidity;
  }
};

#endif // CSMS_H
