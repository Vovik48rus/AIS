#ifndef CSMS_MODBUS_GET_DATA_H
#define CSMS_MODBUS_GET_DATA_H

#include <Arduino.h>
#include "CSMSModbus.h"

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

public:
    // Конструктор — принимает указатель на CSMSModbus и инициализирует значения
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
            Serial.print("Humidity updated: ");
            Serial.println(myCSMSModbusData->humidityPtr);
        }
        if(sensor->getMedium() != myCSMSModbusData->medium)
        {
            myCSMSModbusData->medium = sensor->getMedium();
            Serial.print("Medium updated: ");
            Serial.println(myCSMSModbusData->medium);
        }
    }

    // Опрос: обновить значения, считав их у сенсора
    void poll()
    {
        if (!sensor)
            return;
        updateHumidity();
        if (sensor->getDrySoilValue() != myCSMSModbusData->drySoilValuePtr)
        {
            myCSMSModbusData->drySoilValuePtr = sensor->getDrySoilValue();
            Serial.print("Dry Soil Value updated: ");
            Serial.println(myCSMSModbusData->drySoilValuePtr);
        }
        if (sensor->getWetSoilValue() != myCSMSModbusData->wetSoilValuePtr)
        {
            myCSMSModbusData->wetSoilValuePtr = sensor->getWetSoilValue();
            Serial.print("Wet Soil Value updated: ");
            Serial.println(myCSMSModbusData->wetSoilValuePtr);
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

    // // При необходимости — геттеры по значению:
    // int getHumidity() const { return humidity; }
    // unsigned int getDrySoilValue() const { return drySoilValue; }
    // unsigned int getWetSoilValue() const { return wetSoilValue; }
};

#endif // CSMS_MODBUS_GET_DATA_H
