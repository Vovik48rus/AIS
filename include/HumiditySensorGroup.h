#ifndef HUMIDITY_SENSOR_GROUP_H
#define HUMIDITY_SENSOR_GROUP_H

#include "IHumiditySensor.h"
#include "ICSMS.h"
#include <map>
#include <cstring> // для strncpy

class HumiditySensorGroup : public IHumiditySensor
{
private:
    char name[14];
    std::map<std::string, ICSMS *> sensors;

public:
    explicit HumiditySensorGroup(const char *groupName)
    {
        strncpy(name, groupName, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
    }

    const char *getName() const override
    {
        return name;
    }

    void addSensor(ICSMS *sensor)
    {
        const char* sensorName = sensor->getName();
        if (sensor && sensorName)
        {
            sensors[sensorName] = sensor;
        }
    }

    bool removeSensor(const char *sensorName)
    {
        if (sensorName)
        {
            return sensors.erase(sensorName) > 0;
        }
    }

    bool humidityIsValid() const override {
        for (const auto& pair : sensors) {
            if (pair.second->humidityIsValid()) {
                return true;
            }
        }
        return false;
    }

    int getHumidity() const override {
        int total = 0;
        int count = 0;
        for (const auto& pair : sensors) {
            if (pair.second->humidityIsValid()) {
                total += pair.second->getHumidity();
                ++count;
            }
        }
        return count > 0 ? total / count : -1;
    }

    unsigned int getDrySoilValue(const char *sensorName) const
    {
        auto it = sensors.find(sensorName);
        return (it != sensors.end()) ? it->second->getDrySoilValue() : 0;
    }

    bool setDrySoilValue(const char *sensorName, unsigned int drySoilValue)
    {
        auto it = sensors.find(sensorName);
        return (it != sensors.end()) ? it->second->setDrySoilValue(drySoilValue) : false;
    }

    unsigned int getWetSoilValue(const char *sensorName) const
    {
        auto it = sensors.find(sensorName);
        return (it != sensors.end()) ? it->second->getWetSoilValue() : 0;
    }

    bool setWetSoilValue(const char *sensorName, unsigned int wetSoilValue)
    {
        auto it = sensors.find(sensorName);
        return (it != sensors.end()) ? it->second->setWetSoilValue(wetSoilValue) : false;
    }
};

#endif // HUMIDITY_SENSOR_GROUP_H
