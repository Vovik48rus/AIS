#ifndef POT_GET_DATA_H
#define POT_GET_DATA_H

#include <Arduino.h>
#include "Pot.h"
#include "Logger.h"

const char *stateToString(PotState state)
{
    switch (state)
    {
    case PotState::STATE_SURVEY:
        return "SURVEY";
    case PotState::STATE_WATERING:
        return "WATERING";
    case PotState::STATE_ABSORPTION:
        return "ABSORPTION";
    default:
        return "Unknown";
    }
}

struct PotData
{
    PotState state;    // Текущее состояние горшка
    String stateStr;   // Строковое представление состояния
    int threshold;     
    int surveyTime;    
    int wateringTime;  
    int absorptionTime;
    int lastHumidity;  
    String name;       // Имя горшка

    PotData(PotState state = PotState::STATE_SURVEY,
            int threshold = 0,
            int surveyTime = 0,
            int wateringTime = 0,
            int absorptionTime = 0,
            int lastHumidity = -1,
            String name = "")
        : state(state),
          threshold(threshold),
          surveyTime(surveyTime),
          wateringTime(wateringTime),
          absorptionTime(absorptionTime),
          lastHumidity(lastHumidity),
          name(name)
    {
        stateStr = stateToString(state);
    }
};

class PotGetData
{
private:
    Pot *pot;
    PotData *myPotData;
    Logger *logger;

    void logChange(LevelLog level, const String &message)
    {
        String fullMessage = "[PotData] [Pot: " + myPotData->name + "] " + message;
        logger->send(level, fullMessage.c_str()); // преобразуем в const char*
    }

public:
    PotGetData(Pot *pot, Logger *logger)
        : pot(pot), logger(logger)
    {
        if (pot)
        {
            myPotData = new PotData(
                pot->getState(),
                pot->getThreshold(),
                pot->getSurveyTime(),
                pot->getWateringTime(),
                pot->getAbsorptionTime(),
                pot->getHumidity(),
                pot->getName());
        }
        else
        {
            myPotData = new PotData();
        }
    }

    void updateState()
    {
        if (!pot)
            return;

        // Проверка изменения состояния
        if (pot->getState() != myPotData->state)
        {
            myPotData->state = pot->getState();
            myPotData->stateStr = stateToString(myPotData->state);
            logChange(LevelLog::INFO, "state changed to: " + myPotData->stateStr);
        }
    }

    void updateHumidity()
    {
        if (!pot)
            return;

        // Проверка изменения влажности
        int currentHumidity = pot->getHumidity();
        if (currentHumidity != myPotData->lastHumidity)
        {
            myPotData->lastHumidity = currentHumidity;
            logChange(LevelLog::INFO, "humidity updated: " + String(currentHumidity) + "%");
        }
    }

    void poll()
    {
        if (!pot)
            return;

        updateState();

        updateHumidity();

        // Проверка изменения порога
        if (pot->getThreshold() != myPotData->threshold)
        {
            myPotData->threshold = pot->getThreshold();
            logChange(LevelLog::INFO, "threshold updated: " + String(myPotData->threshold) + "%");
        }

        // Проверка изменения времени опроса
        if (pot->getSurveyTime() != myPotData->surveyTime)
        {
            myPotData->surveyTime = pot->getSurveyTime();
            logChange(LevelLog::INFO, "survey time updated: " + String(myPotData->surveyTime) + " ms");
        }

        // Проверка изменения времени полива
        if (pot->getWateringTime() != myPotData->wateringTime)
        {
            myPotData->wateringTime = pot->getWateringTime();
            logChange(LevelLog::INFO, "watering time updated: " + String(myPotData->wateringTime) + " ms");
        }

        // Проверка изменения времени поглощения
        if (pot->getAbsorptionTime() != myPotData->absorptionTime)
        {
            myPotData->absorptionTime = pot->getAbsorptionTime();
            logChange(LevelLog::INFO, "absorption time updated: " + String(myPotData->absorptionTime) + " ms");
        }
    }

    PotData *getData()
    {
        return myPotData;
    }

    Pot *getPot() const
    {
        return pot;
    }
};

#endif // POT_GET_DATA_H
