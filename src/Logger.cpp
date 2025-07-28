#include "Logger.h"

Logger logger(LevelLog::INFO);

Logger::Logger(LevelLog level) : levelLog(level) {}

LevelLog Logger::getLevelLog()
{
  return levelLog;
}

void Logger::setLevelLog(LevelLog level)
{
  levelLog = level;
}

const char *Logger::levelToString(LevelLog level)
{
  switch (level)
  {
  case INFO:
    return "INFO";
  case DEBUG:
    return "DEBUG";
  case WARNING:
    return "WARNING";
  case CRITICAL_WARNING:
    return "CRITICAL WARNING";
  case ERROR:
    return "ERROR";
  default:
    return "UNKNOWN";
  }
}

void Logger::send(LevelLog msgLevel, const char *message)
{
  // Проверяем, инициализирован ли Serial и готов ли
  if (!Serial)
  {
    return; // интерфейс недоступен — не пишем
  }

  if (msgLevel >= levelLog)
  {
    Serial.print("[");
    Serial.print(levelToString(msgLevel));
    Serial.print("] ");
    Serial.println(message);
  }
}

void Logger::flush()
{
  if (Serial)
    Serial.flush();
}