#ifndef CSMS_MODBUS_GET_DATA_H
#define CSMS_MODBUS_GET_DATA_H

#include <Arduino.h>
#include "CSMSModbus.h"
#include "Logger.h" // Подключаем логгер

struct CSMSModbusData
{
    int humidityPtr;              // Локально сохранённое значение влажности
    unsigned int drySoilValuePtr; // Локально сохранённое значение сухой почвы
    unsigned int wetSoilValuePtr; // Локально сохранённое значение влажной почвы
    int medium;

    CSMSModbusData(int humidity = -1, unsigned int drySoilValue = 0, unsigned int wetSoilValue = 0)
        : humidityPtr(humidity), drySoilValuePtr(drySoilValue), wetSoilValuePtr(wetSoilValue), medium(0) {}
};

class CSMSModbusGetData
{
private:
    CSMSModbus *sensor; // Указатель на внешний объект CSMSModbus
    CSMSModbusData *myCSMSModbusData;

    void logValue(const char* label, int value)
    {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%s: %d", label, value);
        logger.send(WARNING, buffer);
    }

public:
    CSMSModbusGetData(CSMSModbus *sensor)
        : sensor(sensor)
    {
        myCSMSModbusData = new CSMSModbusData(-1, 0, 0);
        if (sensor)
        {
            myCSMSModbusData->drySoilValuePtr = sensor->getDrySoilValue();
            myCSMSModbusData->wetSoilValuePtr = sensor->getWetSoilValue();
        }
    }

    void updateHumidity()
    {
        if (!sensor)
            return;

        if (sensor->getHumidity() != myCSMSModbusData->humidityPtr)
        {
            myCSMSModbusData->humidityPtr = sensor->getHumidity();
            logValue("Humidity updated", myCSMSModbusData->humidityPtr);
        }

        if (sensor->getMedium() != myCSMSModbusData->medium)
        {
            myCSMSModbusData->medium = sensor->getMedium();
            logValue("Medium updated", myCSMSModbusData->medium);
        }
    }

    void poll()
    {
        if (!sensor)
            return;

        updateHumidity();

        if (sensor->getDrySoilValue() != myCSMSModbusData->drySoilValuePtr)
        {
            myCSMSModbusData->drySoilValuePtr = sensor->getDrySoilValue();
            logValue("Dry Soil Value updated", myCSMSModbusData->drySoilValuePtr);
        }

        if (sensor->getWetSoilValue() != myCSMSModbusData->wetSoilValuePtr)
        {
            myCSMSModbusData->wetSoilValuePtr = sensor->getWetSoilValue();
            logValue("Wet Soil Value updated", myCSMSModbusData->wetSoilValuePtr);
        }
    }

    CSMSModbusData *getData()
    {
        return myCSMSModbusData;
    }

    CSMSModbus *getSensor() const
    {
        return sensor;
    }
};

#endif // CSMS_MODBUS_GET_DATA_H
