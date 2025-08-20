#ifndef PUMP_GET_DATA_H
#define PUMP_GET_DATA_H

#include <Arduino.h>
#include "Pump.h"
#include "Logger.h"

struct PumpData
{
    bool isOn;       // Состояние помпы (вкл/выкл)
    String stateStr; // Строковое представление состояния
    int pin;         // Пин, к которому подключена помпа

    PumpData(bool isOn = false, int pin = -1)
        : isOn(isOn), pin(pin)
    {
        stateStr = isOn ? "ON" : "OFF";
    }
};

class PumpGetData
{
private:
    Pump *pump;
    PumpData *myPumpData;
    Logger *logger;

    void logChange(LevelLog level, const String &message)
    {
        String fullMessage = "[PumpData] " + message;
        logger->send(level, fullMessage.c_str());
    }

public:
    PumpGetData(Pump *pump, Logger *logger)
        : pump(pump), logger(logger)
    {
        if (pump)
        {
            myPumpData = new PumpData(
                pump->isOn(),
                pump->getPin()
            );
        }
        else
        {
            myPumpData = new PumpData();
        }
    }

    void updateState()
    {
        if (!pump)
            return;

        bool currentState = pump->isOn();
        if (currentState != myPumpData->isOn)
        {
            myPumpData->isOn = currentState;
            myPumpData->stateStr = currentState ? "ON" : "OFF";
            logChange(LevelLog::INFO, "state changed to: " + myPumpData->stateStr);
        }
    }

    void poll()
    {
        if (!pump)
            return;

        updateState();
    }

    PumpData *getData()
    {
        return myPumpData;
    }

    Pump *getPump() const
    {
        return pump;
    }
};

#endif // PUMP_GET_DATA_H
