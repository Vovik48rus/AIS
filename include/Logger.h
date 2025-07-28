#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <string>

enum LevelLog
{
  DEBUG,
  INFO,
  WARNING,
  CRITICAL_WARNING,
  ERROR
};

class Logger
{
private:
  LevelLog levelLog;
  const char* levelToString(LevelLog level);

public:
  Logger(LevelLog level);
  LevelLog getLevelLog();
  void setLevelLog(LevelLog level);
  void send(LevelLog msgLevel, const char* message);
  void flush();
};

// Declare the global logger pointer
extern Logger logger;

#endif // LOGGER_H
