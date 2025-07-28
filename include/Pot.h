#ifndef POT_H
#define POT_H

#include "Logger.h"
#include "ICSMS.h"
#include <Looper.h>
#include "Pump.h"

using namespace std;

enum PotState
{
  STATE_SURVEY,
  STATE_WATERING,
  STATE_ABSORPTION
};

class Pot : public LoopTimerBase
{
  string name;

  Logger *logger;
  ICSMS *myCSMS;
  Pump *pump;
  int threshold;      // порог срабатывания полива, влажность %
  int surveyTime;     // период опроса
  int wateringTime;   // время полива
  int absorptionTime; // время поглощения почвы
  PotState potState = PotState::STATE_SURVEY;

public:
  Pot(string name, Logger *logger, Pump *pump, ICSMS *myCSMS, int threshold, int surveyTime, int wateringTime, int absorptionTime)
      : LoopTimerBase(name.c_str(), surveyTime), name(name), logger(logger), pump(pump), myCSMS(myCSMS), threshold(threshold), surveyTime(surveyTime), wateringTime(wateringTime), absorptionTime(absorptionTime)
  {
    changePotState(PotState::STATE_SURVEY);
  }

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
    logger->send(LevelLog::WARNING, (string("Survey tick [Pot:") + name + string("]")).c_str());

    if (myCSMS->humidityIsValid())
    {
      int humidity = myCSMS->getHumidity();
      logger->send(LevelLog::WARNING, (string("[Pot: " + name + "] " + " Влажность: " + to_string(humidity))).c_str());
      if (humidity <= this->threshold)
      {
        this->pump->on();
        changePotState(PotState::STATE_WATERING);
        logger->send(LevelLog::WARNING, (string("Начало полива [Pot: ") + name + string("]")).c_str());
      }
      else
      {
        logger->send(LevelLog::WARNING, ("Горшок [Pot: " + name + "] не требует полива").c_str());
      }
    }
    else
    {
      logger->send(LevelLog::WARNING, (string("Влажность с датчика ") + string(myCSMS->getName()) + string(" не является правдивой")).c_str());
    }
  }

  void wateringTick()
  {
    logger->send(LevelLog::WARNING, (string("Конец полива, wait for absorption [Pot: ") + name + string("]")).c_str());
    this->pump->off();
    changePotState(PotState::STATE_ABSORPTION);
  }

  void absorptionTick()
  {
    if (myCSMS->humidityIsValid())
    {
      int humidity = myCSMS->getHumidity();
      logger->send(LevelLog::WARNING, (string("[Pot: " + name + "] " + " Влажность: " + to_string(humidity))).c_str());
      if (humidity <= this->threshold)
      {
        this->pump->on();
        changePotState(PotState::STATE_WATERING);
        logger->send(LevelLog::WARNING, (string("Продолжение полива [Pot: ") + name + string("]")).c_str());
      }
      else
      {
        changePotState(PotState::STATE_SURVEY);
        logger->send(LevelLog::WARNING, (string() + "Absorption done, restart survey [Pot: " + name + "]").c_str());
      }
    }
    else
    {
      logger->send(LevelLog::WARNING, (string("Влажность с датчика ") + string(myCSMS->getName()) + string("не является правдивой")).c_str());
    }
  }

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
};

#endif // POT_H
