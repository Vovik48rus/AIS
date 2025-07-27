#ifndef TIMERSURVEY_H
#define TIMERSURVEY_H

#include "Logger.h"  // Assuming this exists
#include <Looper.h>

using namespace std;

class Pot;

class TimerSurvey : public LoopTimerBase 
{
  Pot* pot;

public:
  TimerSurvey(uint32_t intervalMs, Pot* pot)
    : LoopTimerBase(intervalMs), pot(pot) {}

  void exec() override;
};

#endif // TIMERSURVEY_H
