#include "CSMSModbus.h"

class CSMSModbusTest : public CSMSModbus
{
public:
    CSMSModbusTest(const char *name, Logger *logger, uint8_t slaveId, uint16_t registerAddress, uint32_t normalInterval,
                   unsigned int drySoilValue = 700, unsigned int wetSoilValue = 275, uint32_t repeatInterval = 500)
        : CSMSModbus(name, logger, slaveId, registerAddress, normalInterval, drySoilValue, wetSoilValue, repeatInterval),
          startTime(millis())
    {
    }

protected:
    mutable unsigned long startTime;

    long readInputRegister() const override
    {
        // Прошло времени с момента старта (в секундах)
        float elapsed = (millis() - startTime) / 1000.0f;

        // Полный цикл (вверх и вниз) = 10 секунд
        float phase = fmod(elapsed, 10.0f) / 10.0f; 

        // Линейное изменение: 0..1..0
        float factor = phase < 0.5f ? (phase * 2.0f) : (2.0f - phase * 2.0f);

        // Интерполяция между drySoilValue и wetSoilValue
        return (long)(getWetSoilValue() + factor * (getDrySoilValue() - getWetSoilValue()));
    }
};
