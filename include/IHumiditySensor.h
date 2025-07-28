#ifndef I_HUMIDITY_SENSOR_H
#define I_HUMIDITY_SENSOR_H

class IHumiditySensor {
public:
  virtual ~IHumiditySensor() {}

  virtual const char* getName() const = 0;
  virtual bool humidityIsValid() const = 0;
  virtual int getHumidity() const = 0;
};

#endif // I_HUMIDITY_SENSOR_H
