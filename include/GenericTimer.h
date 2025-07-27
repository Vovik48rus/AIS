#ifndef GENERICTIMER_H
#define GENERICTIMER_H

#include <Looper.h>
#include <functional>

class GenericTimer : public LoopTimerBase {
  std::function<void()> callback;

public:
  GenericTimer(uint32_t intervalMs, std::function<void()> cb)
    : LoopTimerBase(intervalMs), callback(cb) {}

  void exec() override {
    if (callback) callback();
  }

  void updateCallback(std::function<void()> cb) {
    callback = cb;
  }

  // void updateInterval(uint32_t intervalMs) {
  //   this->interval = intervalMs;
  // }
};

#endif // GENERICTIMER_H
