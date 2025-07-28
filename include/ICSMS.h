#ifndef ICSMS_H
#define ICSMS_H

#include <Looper.h>

class ICSMS : private LoopTimerBase
{
public:
    ICSMS(const char *id, uint32_t intervalMs) : LoopTimerBase(id, intervalMs) {}

    virtual bool humidityIsValid() const = 0;
    virtual int getHumidity() const = 0;

    virtual const char *getName() const = 0;

    virtual unsigned int getDrySoilValue() const = 0;
    virtual bool setDrySoilValue(unsigned int drySoilValue) = 0;

    virtual unsigned int getWetSoilValue() const = 0;
    virtual bool setWetSoilValue(unsigned int wetSoilValue) = 0;

private:
    virtual void exec() = 0;
};

#endif // ICSMS_H
