#ifndef RPLidar_h
#define RPLidar_h

#include <SPI.h>

class RPLidar
{
  public:
    RPLidar();
    RPLidar(uint8_t pin);
    uint8_t _pwm_pin;

    IntervalTimer RotationSpoofTimer;

    double RotRPM = 10;
    unsigned int RotTabCounter;
    bool RotTabState = LOW;

    void init();
    void run();
    void update(float dps);
};

#endif
