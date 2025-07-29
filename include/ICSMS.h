#ifndef ICSMS_H
#define ICSMS_H

#include <Looper.h>          // LoopTimerBase
#include "IHumiditySensor.h" // Новый интерфейс

class ICSMS : public IHumiditySensor, private LoopTimerBase
{
public:
    ICSMS(const char *id, uint32_t intervalMs)
        : LoopTimerBase(id, intervalMs) {}

    virtual unsigned int getDrySoilValue() const = 0;
    virtual bool setDrySoilValue(unsigned int drySoilValue) = 0;

    virtual unsigned int getWetSoilValue() const = 0;
    virtual bool setWetSoilValue(unsigned int wetSoilValue) = 0;

    virtual bool calibrateDrySoilValue() = 0;
    virtual bool calibrateWetSoilValue() = 0;

    virtual int getMedium() = 0;

private:
    virtual void exec() = 0;
};

#endif // ICSMS_H
