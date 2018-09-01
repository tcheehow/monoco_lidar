#include "EM7180.h"
#include "RPLidar.h"

EM7180 imu(I2C_PINS_7_8, 17);
RPLidar rplidar(14);
pose_msg_t pose;

void setup()
{
  imu.init();
  rplidar.init();
  rplidar.RotationSpoofTimer.begin(rplidar_inthandler, 6000);
  attachInterrupt(imu._int_pin, myinthandler, RISING);  // define interrupt for INT pin output of EM7180
}

void loop()
{
//  imu.defaultEM7180();/
  pose = imu.getSentralRPY();
  rplidar.update(abs(pose.twist[2]));
//  Serial.println(rplidar.RotRPM);
}

void myinthandler()
{
  imu.newData = true;
}

void rplidar_inthandler()
{
  rplidar.run();
}
