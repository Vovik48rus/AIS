#ifndef CSMS_MODBUS_H
#define CSMS_MODBUS_H

#include <Arduino.h>
#include <ArduinoRS485.h>
#include <ArduinoModbus.h>

#include "ICSMS.h"
#include "Logger.h"

class CSMSModbus : public ICSMS
{
private:
    uint8_t slaveId;
    uint16_t registerAddress;
    const char *name;
    uint32_t lastValueRead = 0;
    int humidity = -1;
    unsigned int drySoilValue;
    unsigned int wetSoilValue;

    Logger *logger;

    int failCount = 0;                    // количество последовательных неудач
    static constexpr int MAX_FAILS = 5;   // максимум последовательных неудач до сброса humidity
    static constexpr int MAX_RETRIES = 3; // количество попыток при одном опросе

public:
    CSMSModbus(const char *name, Logger *logger, uint8_t slaveId, uint16_t registerAddress, uint32_t intervalMs,
               unsigned int drySoilValue = 700, unsigned int wetSoilValue = 275)
        : ICSMS(name, intervalMs),
          slaveId(slaveId),
          registerAddress(registerAddress),
          name(name),
          logger(logger),
          drySoilValue(drySoilValue),
          wetSoilValue(wetSoilValue)
    {
        logger->send(LevelLog::INFO, (String("Modbus CSMS sensor initialized: ") + name).c_str());
    }

    void exec() override
    {
        int attempt = 0;
        int value = -1;

        while (attempt < MAX_RETRIES)
        {
            logger->flush();
            value = ModbusRTUClient.inputRegisterRead(slaveId, registerAddress);
            if (value != -1)
                break; // успех

            attempt++;
            logger->send(LevelLog::WARNING, (String("Attempt #") + attempt + " failed for [" + name + "], error: " + ModbusRTUClient.lastError()).c_str());
            // delay(100);
        }

        if (value == -1)
        {
            failCount++;
            logger->send(LevelLog::WARNING, (String("All retries failed for [") + name + "], fail count = " + failCount).c_str());

            if (failCount >= MAX_FAILS)
            {
                humidity = -1;
                logger->send(LevelLog::ERROR, (String("Max fail count reached for ") + name + ", setting humidity = -1").c_str());
                failCount = 0;
            }

            return;
        }

        failCount = 0;
        lastValueRead = value;
        humidity = map(value, drySoilValue, wetSoilValue, 0, 100);
        logger->send(LevelLog::WARNING, (String("Sensor [") + name + "] raw: " + value + ", humidity: " + humidity + "%").c_str());
    }

    bool humidityIsValid() const override
    {
        return humidity >= 0 && humidity <= 100;
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
        if (value < 1024)
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
        if (value < 1024)
        {
            wetSoilValue = value;
            return true;
        }
        logger->send(LevelLog::WARNING, (String("Invalid wetSoilValue set attempt on ") + name).c_str());
        return false;
    }

    ~CSMSModbus() {}
};

#endif // CSMS_MODBUS_H
