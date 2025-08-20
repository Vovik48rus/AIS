#ifndef POT_H
#define POT_H

#include "Logger.h"
#include "IHumiditySensor.h"
#include <Looper.h>
#include "Pump.h"

enum PotState
{
  STATE_SURVEY,
  STATE_WATERING,
  STATE_ABSORPTION
};

class Pot : public LoopTimerBase
{
  String name;

  Logger *logger;
  IHumiditySensor *myCSMS;
  Pump *pump;
  int threshold;      // порог срабатывания полива, влажность %
  int surveyTime;     // период опроса
  int wateringTime;   // время полива
  int absorptionTime; // время поглощения почвы
  PotState potState = PotState::STATE_SURVEY;

public:
  Pot(String name, Logger *logger, Pump *pump, IHumiditySensor *myCSMS, int threshold, int surveyTime, int wateringTime, int absorptionTime)
      : LoopTimerBase(name.c_str(), surveyTime),
        name(name),
        logger(logger),
        pump(pump),
        myCSMS(myCSMS),
        threshold(threshold),
        surveyTime(surveyTime),
        wateringTime(wateringTime),
        absorptionTime(absorptionTime)
  {
    changePotState(PotState::STATE_SURVEY);
  }

  String getName() const { return name; }
  const char* getNameCStr() const { return name.c_str(); }
  int getThreshold() const { return threshold; }
  int getSurveyTime() const { return surveyTime; }
  int getWateringTime() const { return wateringTime; }
  int getAbsorptionTime() const { return absorptionTime; }
  PotState getState() const { return potState; }

  int getHumidity() const
  {
    if (myCSMS)
      return myCSMS->getHumidity();
    return -1;
  }

  bool humidityIsValid() const
  {
    if (myCSMS)
      return myCSMS->humidityIsValid();
    return false;
  }

  Pump *getPump() const { return pump; }
  IHumiditySensor *getSensor() const { return myCSMS; }

  void setThreshold(int value) { threshold = value; }
  void setSurveyTime(int value)
  {
    surveyTime = value;
    restartIfState(PotState::STATE_SURVEY);
  }
  void setWateringTime(int value)
  {
    wateringTime = value;
    restartIfState(PotState::STATE_WATERING);
  }
  void setAbsorptionTime(int value)
  {
    absorptionTime = value;
    restartIfState(PotState::STATE_ABSORPTION);
  }

  void setState(PotState newState) { changePotState(newState); }

  void exec() override
  {
    switch (this->potState)
    {
    case PotState::STATE_SURVEY:
      surveyTick();
      break;
    case PotState::STATE_WATERING:
      wateringTick();
      break;
    case PotState::STATE_ABSORPTION:
      absorptionTick();
      break;
    default:
      break;
    }
  }

  void surveyTick()
  {
    logger->send(LevelLog::WARNING, ("Survey tick [Pot:" + name + "]").c_str());

    if (myCSMS->humidityIsValid())
    {
      int humidity = myCSMS->getHumidity();
      logger->send(LevelLog::WARNING, ("[Pot: " + name + "] Влажность: " + String(humidity) + "%").c_str());

      if (humidity <= this->threshold)
      {
        this->pump->on();
        changePotState(PotState::STATE_WATERING);
        logger->send(LevelLog::WARNING, ("Начало полива [Pot: " + name + "]").c_str());
      }
      else
      {
        logger->send(LevelLog::WARNING, ("Горшок [Pot: " + name + "] не требует полива").c_str());
      }
    }
    else
    {
      logger->send(LevelLog::WARNING, ("Влажность с датчика " + String(myCSMS->getName()) + " не является правдивой").c_str());
    }
  }

  void wateringTick()
  {
    logger->send(LevelLog::WARNING, ("Конец полива, wait for absorption [Pot: " + name + "]").c_str());
    this->pump->off();
    changePotState(PotState::STATE_ABSORPTION);
  }

  void absorptionTick()
  {
    if (myCSMS->humidityIsValid())
    {
      int humidity = myCSMS->getHumidity();
      logger->send(LevelLog::WARNING, ("[Pot: " + name + "] Влажность: " + String(humidity) + "%").c_str());

      if (humidity <= this->threshold)
      {
        this->pump->on();
        changePotState(PotState::STATE_WATERING);
        logger->send(LevelLog::WARNING, ("Продолжение полива [Pot: " + name + "]").c_str());
      }
      else
      {
        changePotState(PotState::STATE_SURVEY);
        logger->send(LevelLog::WARNING, ("Absorption done, restart survey [Pot: " + name + "]").c_str());
      }
    }
    else
    {
      logger->send(LevelLog::WARNING, ("Влажность с датчика " + String(myCSMS->getName()) + " не является правдивой").c_str());
    }
  }

private:
  void changePotState(PotState potState)
  {
    switch (potState)
    {
    case PotState::STATE_SURVEY:
      this->restart(surveyTime);
      break;
    case PotState::STATE_WATERING:
      this->restart(wateringTime);
      break;
    case PotState::STATE_ABSORPTION:
      this->restart(absorptionTime);
      break;
    default:
      break;
    }
    this->potState = potState;
  }

  void restartIfState(PotState state)
  {
    if (this->potState == state)
    {
      changePotState(state);
    }
  }
};

#endif // POT_H
