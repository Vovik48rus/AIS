    #ifndef PUMP_H
    #define PUMP_H

    #include "Logger.h"
    #include <Looper.h>

    class Pump
    {
    private:
        int pin;
        bool isEnable = false;

    public:
        Pump(int pin): pin(pin)
        {
            pinMode(pin, OUTPUT);
            this->off();
        }

        void on()
        {
            logger.send(LevelLog::WARNING, "[Pump] Включение помпы ");
            digitalWrite(pin, HIGH);
            this->isEnable = true;
        }

        void off()
        {
            logger.send(LevelLog::WARNING, "[Pump] Выключение помпы ");
            digitalWrite(pin, LOW);
            this->isEnable = false;
        }

        bool isOn()
        {
            return this->isEnable;
        }

        int getPin()
        {
            return this->pin;
        }
    };

    #endif // PUMP_H
