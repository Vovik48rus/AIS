#include "TimerSurvey.h"
#include "Pot.h"

void TimerSurvey::exec() {
  pot->surveyTick();
}
