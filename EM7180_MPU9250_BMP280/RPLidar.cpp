#include "RPLidar.h"

RPLidar::RPLidar()
{
  RPLidar(14);
}

RPLidar::RPLidar(uint8_t pin)
{
  _pwm_pin = pin;
}

void RPLidar::init() {
  pinMode(_pwm_pin , OUTPUT);
  digitalWrite(_pwm_pin, LOW);

//  RotationSpoofTimer.begin(run, 6000);
}

void RPLidar::run() {
  if (RotTabCounter < 14)
  {
    if (RotTabState == HIGH)
    {
      RotTabState = LOW;
      digitalWrite(14, LOW);
      RotTabCounter = RotTabCounter + 1;
    }
    else
    {
      RotTabState = HIGH;
      digitalWrite(14, HIGH);
    }
    RotationSpoofTimer.update(((1 / RotRPM) / 30.0) * 1000000.0);
//    Serial.println(RotTabCounter);
  }
  else
  {
    if (RotTabState == HIGH)
    {
      RotTabState = LOW;
      digitalWrite(14, LOW);
      RotationSpoofTimer.update(((((1 / RotRPM) / 30.0) * 1000000.0) / 2.0) * 3.0);
      RotTabCounter = 0;
    }
    else
    {
      RotTabState = HIGH;
      digitalWrite(14, HIGH);
      RotationSpoofTimer.update((((1 / RotRPM) / 30.0) * 1000000.0) / 2.0);
    }
  }
}

void RPLidar::update(float dps){
  float rpm = dps / 360.0;
  RotRPM = rpm;
}

