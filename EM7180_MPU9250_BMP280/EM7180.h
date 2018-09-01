/* EM7180_MPU9250_BMP280_t3 Basic Example Code
  by: Kris Winer
  date: September 11, 2015
  license: Beerware - Use this code however you'd like. If you
  find it useful you can buy me a beer some time.

  The EM7180 SENtral sensor hub is not a motion sensor, but rather takes raw sensor data from a variety of motion sensors,
  in this case the MPU9250 (with embedded MPU9250 + AK8963C), and does sensor fusion with quaternions as its output. The SENtral loads firmware from the
  on-board M24512DRC 512 kbit EEPROM upon startup, configures and manages the sensors on its dedicated master I2C bus,
  and outputs scaled sensor data (accelerations, rotation rates, and magnetic fields) as well as quaternions and
  heading/pitch/roll, if selected.

  This sketch demonstrates basic EM7180 SENtral functionality including parameterizing the register addresses, initializing the sensor,
  getting properly scaled accelerometer, gyroscope, and magnetometer data out. Added display functions to
  allow display to on breadboard monitor. Addition of 9 DoF sensor fusion using open source Madgwick and
  Mahony filter algorithms to compare with the hardware sensor fusion results.
  Sketch runs on the 3.3 V 8 MHz Pro Mini and the Teensy 3.1.

  This sketch is specifically for the Teensy 3.1 Mini Add-On shield with the EM7180 SENtral sensor hub as master,
  the MPU9250 9-axis motion sensor (accel/gyro/mag) as slave, a BMP280 pressure/temperature sensor, and an M24512DRC
  512kbit (64 kByte) EEPROM as slave all connected via I2C. The SENtral can use the pressure data in the sensor fusion
  yet and there is a driver for the BMP280 in the SENtral firmware.

  This sketch uses SDA/SCL on pins 17/16, respectively, and it uses the Teensy 3.1-specific Wire library i2c_t3.h.
  The BMP280 is a simple but high resolution pressure sensor, which can be used in its high resolution
  mode but with power consumption of 20 microAmp, or in a lower resolution mode with power consumption of
  only 1 microAmp. The choice will depend on the application.

  SDA and SCL should have external pull-up resistors (to 3.3V).
  4k7 resistors are on the EM7180+MPU9250+BMP280+M24512DRC Mini Add-On board for Teensy 3.1.

  Hardware setup:
  EM7180 Mini Add-On ------- Teensy 3.1
  VDD ---------------------- 3.3V
  SDA ----------------------- 17
  SCL ----------------------- 16
  GND ---------------------- GND
  INT------------------------ 8

  Note: All the sensors n this board are I2C sensor and uses the Teensy 3.1 i2c_t3.h Wire library.
  Because the sensors are not 5V tolerant, we are using a 3.3 V 8 MHz Pro Mini or a 3.3 V Teensy 3.1.
*/


#ifndef EM7180_h
#define EM7180_h

//#include "Wire.h"
#include <i2c_t3.h>
#include <SPI.h>
#include <avr/io.h>
#include <avr/interrupt.h>

// BMP280 registers
#define BMP280_TEMP_XLSB  0xFC
#define BMP280_TEMP_LSB   0xFB
#define BMP280_TEMP_MSB   0xFA
#define BMP280_PRESS_XLSB 0xF9
#define BMP280_PRESS_LSB  0xF8
#define BMP280_PRESS_MSB  0xF7
#define BMP280_CONFIG     0xF5
#define BMP280_CTRL_MEAS  0xF4
#define BMP280_STATUS     0xF3
#define BMP280_RESET      0xE0
#define BMP280_ID         0xD0  // should be 0x58
#define BMP280_CALIB00    0x88

// See also MPU-9250 Register Map and Descriptions, Revision 4.0, RM-MPU-9250A-00, Rev. 1.4, 9/9/2013 for registers not listed in
// above document; the MPU9250 and MPU9150 are virtually identical but the latter has a different register map
//
//Magnetometer Registers
#define AK8963_ADDRESS   0x0C
#define WHO_AM_I_AK8963  0x00 // should return 0x48
#define INFO             0x01
#define AK8963_ST1       0x02  // data ready status bit 0
#define AK8963_XOUT_L   0x03  // data
#define AK8963_XOUT_H  0x04
#define AK8963_YOUT_L  0x05
#define AK8963_YOUT_H  0x06
#define AK8963_ZOUT_L  0x07
#define AK8963_ZOUT_H  0x08
#define AK8963_ST2       0x09  // Data overflow bit 3 and data read error status bit 2
#define AK8963_CNTL      0x0A  // Power down (0000), single-measurement (0001), self-test (1000) and Fuse ROM (1111) modes on bits 3:0
#define AK8963_ASTC      0x0C  // Self test control
#define AK8963_I2CDIS    0x0F  // I2C disable
#define AK8963_ASAX      0x10  // Fuse ROM x-axis sensitivity adjustment value
#define AK8963_ASAY      0x11  // Fuse ROM y-axis sensitivity adjustment value
#define AK8963_ASAZ      0x12  // Fuse ROM z-axis sensitivity adjustment value

#define SELF_TEST_X_GYRO 0x00
#define SELF_TEST_Y_GYRO 0x01
#define SELF_TEST_Z_GYRO 0x02

/*#define X_FINE_GAIN      0x03 // [7:0] fine gain
  #define Y_FINE_GAIN      0x04
  #define Z_FINE_GAIN      0x05
  #define XA_OFFSET_H      0x06 // User-defined trim values for accelerometer
  #define XA_OFFSET_L_TC   0x07
  #define YA_OFFSET_H      0x08
  #define YA_OFFSET_L_TC   0x09
  #define ZA_OFFSET_H      0x0A
  #define ZA_OFFSET_L_TC   0x0B */

#define SELF_TEST_X_ACCEL 0x0D
#define SELF_TEST_Y_ACCEL 0x0E
#define SELF_TEST_Z_ACCEL 0x0F

#define SELF_TEST_A      0x10

#define XG_OFFSET_H      0x13  // User-defined trim values for gyroscope
#define XG_OFFSET_L      0x14
#define YG_OFFSET_H      0x15
#define YG_OFFSET_L      0x16
#define ZG_OFFSET_H      0x17
#define ZG_OFFSET_L      0x18
#define SMPLRT_DIV       0x19
#define CONFIG           0x1A
#define GYRO_CONFIG      0x1B
#define ACCEL_CONFIG     0x1C
#define ACCEL_CONFIG2    0x1D
#define LP_ACCEL_ODR     0x1E
#define WOM_THR          0x1F

#define MOT_DUR          0x20  // Duration counter threshold for motion interrupt generation, 1 kHz rate, LSB = 1 ms
#define ZMOT_THR         0x21  // Zero-motion detection threshold bits [7:0]
#define ZRMOT_DUR        0x22  // Duration counter threshold for zero motion interrupt generation, 16 Hz rate, LSB = 64 ms

#define FIFO_EN          0x23
#define I2C_MST_CTRL     0x24
#define I2C_SLV0_ADDR    0x25
#define I2C_SLV0_REG     0x26
#define I2C_SLV0_CTRL    0x27
#define I2C_SLV1_ADDR    0x28
#define I2C_SLV1_REG     0x29
#define I2C_SLV1_CTRL    0x2A
#define I2C_SLV2_ADDR    0x2B
#define I2C_SLV2_REG     0x2C
#define I2C_SLV2_CTRL    0x2D
#define I2C_SLV3_ADDR    0x2E
#define I2C_SLV3_REG     0x2F
#define I2C_SLV3_CTRL    0x30
#define I2C_SLV4_ADDR    0x31
#define I2C_SLV4_REG     0x32
#define I2C_SLV4_DO      0x33
#define I2C_SLV4_CTRL    0x34
#define I2C_SLV4_DI      0x35
#define I2C_MST_STATUS   0x36
#define INT_PIN_CFG      0x37
#define INT_ENABLE       0x38
#define DMP_INT_STATUS   0x39  // Check DMP interrupt
#define INT_STATUS       0x3A
#define ACCEL_XOUT_H     0x3B
#define ACCEL_XOUT_L     0x3C
#define ACCEL_YOUT_H     0x3D
#define ACCEL_YOUT_L     0x3E
#define ACCEL_ZOUT_H     0x3F
#define ACCEL_ZOUT_L     0x40
#define TEMP_OUT_H       0x41
#define TEMP_OUT_L       0x42
#define GYRO_XOUT_H      0x43
#define GYRO_XOUT_L      0x44
#define GYRO_YOUT_H      0x45
#define GYRO_YOUT_L      0x46
#define GYRO_ZOUT_H      0x47
#define GYRO_ZOUT_L      0x48
#define EXT_SENS_DATA_00 0x49
#define EXT_SENS_DATA_01 0x4A
#define EXT_SENS_DATA_02 0x4B
#define EXT_SENS_DATA_03 0x4C
#define EXT_SENS_DATA_04 0x4D
#define EXT_SENS_DATA_05 0x4E
#define EXT_SENS_DATA_06 0x4F
#define EXT_SENS_DATA_07 0x50
#define EXT_SENS_DATA_08 0x51
#define EXT_SENS_DATA_09 0x52
#define EXT_SENS_DATA_10 0x53
#define EXT_SENS_DATA_11 0x54
#define EXT_SENS_DATA_12 0x55
#define EXT_SENS_DATA_13 0x56
#define EXT_SENS_DATA_14 0x57
#define EXT_SENS_DATA_15 0x58
#define EXT_SENS_DATA_16 0x59
#define EXT_SENS_DATA_17 0x5A
#define EXT_SENS_DATA_18 0x5B
#define EXT_SENS_DATA_19 0x5C
#define EXT_SENS_DATA_20 0x5D
#define EXT_SENS_DATA_21 0x5E
#define EXT_SENS_DATA_22 0x5F
#define EXT_SENS_DATA_23 0x60
#define MOT_DETECT_STATUS 0x61
#define I2C_SLV0_DO      0x63
#define I2C_SLV1_DO      0x64
#define I2C_SLV2_DO      0x65
#define I2C_SLV3_DO      0x66
#define I2C_MST_DELAY_CTRL 0x67
#define SIGNAL_PATH_RESET  0x68
#define MOT_DETECT_CTRL  0x69
#define USER_CTRL        0x6A  // Bit 7 enable DMP, bit 3 reset DMP
#define PWR_MGMT_1       0x6B // Device defaults to the SLEEP mode
#define PWR_MGMT_2       0x6C
#define DMP_BANK         0x6D  // Activates a specific bank in the DMP
#define DMP_RW_PNT       0x6E  // Set read/write pointer to a specific start address in specified DMP bank
#define DMP_REG          0x6F  // Register in DMP from which to read or to which to write
#define DMP_REG_1        0x70
#define DMP_REG_2        0x71
#define FIFO_COUNTH      0x72
#define FIFO_COUNTL      0x73
#define FIFO_R_W         0x74
#define WHO_AM_I_MPU9250 0x75 // Should return 0x71
#define XA_OFFSET_H      0x77
#define XA_OFFSET_L      0x78
#define YA_OFFSET_H      0x7A
#define YA_OFFSET_L      0x7B
#define ZA_OFFSET_H      0x7D
#define ZA_OFFSET_L      0x7E

// EM7180 SENtral register map
// see http://www.emdeveloper.com/downloads/7180/EMSentral_EM7180_Register_Map_v1_3.pdf
//
#define EM7180_QX                 0x00  // this is a 32-bit normalized floating point number read from registers 0x00-03
#define EM7180_QY                 0x04  // this is a 32-bit normalized floating point number read from registers 0x04-07
#define EM7180_QZ                 0x08  // this is a 32-bit normalized floating point number read from registers 0x08-0B
#define EM7180_QW                 0x0C  // this is a 32-bit normalized floating point number read from registers 0x0C-0F
#define EM7180_QTIME              0x10  // this is a 16-bit unsigned integer read from registers 0x10-11
#define EM7180_MX                 0x12  // int16_t from registers 0x12-13
#define EM7180_MY                 0x14  // int16_t from registers 0x14-15
#define EM7180_MZ                 0x16  // int16_t from registers 0x16-17
#define EM7180_MTIME              0x18  // uint16_t from registers 0x18-19
#define EM7180_AX                 0x1A  // int16_t from registers 0x1A-1B
#define EM7180_AY                 0x1C  // int16_t from registers 0x1C-1D
#define EM7180_AZ                 0x1E  // int16_t from registers 0x1E-1F
#define EM7180_ATIME              0x20  // uint16_t from registers 0x20-21
#define EM7180_GX                 0x22  // int16_t from registers 0x22-23
#define EM7180_GY                 0x24  // int16_t from registers 0x24-25
#define EM7180_GZ                 0x26  // int16_t from registers 0x26-27
#define EM7180_GTIME              0x28  // uint16_t from registers 0x28-29
#define EM7180_Baro               0x2A  // start of two-byte MS5637 pressure data, 16-bit signed interger
#define EM7180_BaroTIME           0x2C  // start of two-byte MS5637 pressure timestamp, 16-bit unsigned
#define EM7180_Temp               0x2E  // start of two-byte MS5637 temperature data, 16-bit signed interger
#define EM7180_TempTIME           0x30  // start of two-byte MS5637 temperature timestamp, 16-bit unsigned
#define EM7180_QRateDivisor       0x32  // uint8_t 
#define EM7180_EnableEvents       0x33
#define EM7180_HostControl        0x34
#define EM7180_EventStatus        0x35
#define EM7180_SensorStatus       0x36
#define EM7180_SentralStatus      0x37
#define EM7180_AlgorithmStatus    0x38
#define EM7180_FeatureFlags       0x39
#define EM7180_ParamAcknowledge   0x3A
#define EM7180_SavedParamByte0    0x3B
#define EM7180_SavedParamByte1    0x3C
#define EM7180_SavedParamByte2    0x3D
#define EM7180_SavedParamByte3    0x3E
#define EM7180_ActualMagRate      0x45
#define EM7180_ActualAccelRate    0x46
#define EM7180_ActualGyroRate     0x47
#define EM7180_ActualBaroRate     0x48
#define EM7180_ActualTempRate     0x49
#define EM7180_ErrorRegister      0x50
#define EM7180_AlgorithmControl   0x54
#define EM7180_MagRate            0x55
#define EM7180_AccelRate          0x56
#define EM7180_GyroRate           0x57
#define EM7180_BaroRate           0x58
#define EM7180_TempRate           0x59
#define EM7180_LoadParamByte0     0x60
#define EM7180_LoadParamByte1     0x61
#define EM7180_LoadParamByte2     0x62
#define EM7180_LoadParamByte3     0x63
#define EM7180_ParamRequest       0x64
#define EM7180_ROMVersion1        0x70
#define EM7180_ROMVersion2        0x71
#define EM7180_RAMVersion1        0x72
#define EM7180_RAMVersion2        0x73
#define EM7180_ProductID          0x90
#define EM7180_RevisionID         0x91
#define EM7180_RunStatus          0x92
#define EM7180_UploadAddress      0x94 // uint16_t registers 0x94 (MSB)-5(LSB)
#define EM7180_UploadData         0x96
#define EM7180_CRCHost            0x97  // uint32_t from registers 0x97-9A
#define EM7180_ResetRequest       0x9B
#define EM7180_PassThruStatus     0x9E
#define EM7180_PassThruControl    0xA0
#define EM7180_ACC_LPF_BW         0x5B  //Register GP36
#define EM7180_GYRO_LPF_BW        0x5C  //Register GP37
#define EM7180_BARO_LPF_BW        0x5D  //Register GP38

#define EM7180_ADDRESS           0x28   // Address of the EM7180 SENtral sensor hub
#define M24512DFM_DATA_ADDRESS   0x50   // Address of the 500 page M24512DRC EEPROM data buffer, 1024 bits (128 8-bit bytes) per page
#define M24512DFM_IDPAGE_ADDRESS 0x58   // Address of the single M24512DRC lockable EEPROM ID page
#define MPU9250_ADDRESS          0x68   // Device address of MPU9250 when ADO = 0
#define AK8963_ADDRESS           0x0C   // Address of magnetometer
#define BMP280_ADDRESS           0x76   // Address of BMP280 altimeter when ADO = 0

#define SerialDebug true  // set to true to get Serial output for debugging

struct pose_msg_t {
  int timestamp;
  float quat[4];
  float euler[3];
  float twist[3];
};

class EM7180
{
  public:
    EM7180();
    EM7180(i2c_pins i2c_pin, uint8_t int_pin);
    void defaultEM7180();

    i2c_pins _i2c_pin = I2C_PINS_7_8;
    uint8_t _int_pin = 17;
    volatile bool newData = false;

    void init();
    pose_msg_t getSentralRPY();

    // Set initial input parameters
    enum Ascale {
      AFS_2G = 0,
      AFS_4G,
      AFS_8G,
      AFS_16G
    };

    enum Gscale {
      GFS_250DPS = 0,
      GFS_500DPS,
      GFS_1000DPS,
      GFS_2000DPS
    };

    enum Mscale {
      MFS_14BITS = 0, // 0.6 mG per LSB
      MFS_16BITS      // 0.15 mG per LSB
    };

    enum Posr {
      P_OSR_00 = 0,  // no op
      P_OSR_01,
      P_OSR_02,
      P_OSR_04,
      P_OSR_08,
      P_OSR_16
    };

    enum Tosr {
      T_OSR_00 = 0,  // no op
      T_OSR_01,
      T_OSR_02,
      T_OSR_04,
      T_OSR_08,
      T_OSR_16
    };

    enum IIRFilter {
      full = 0,  // bandwidth at full sample rate
      BW0_223ODR,
      BW0_092ODR,
      BW0_042ODR,
      BW0_021ODR // bandwidth at 0.021 x sample rate
    };

    enum Mode {
      BMP280Sleep = 0,
      forced,
      forced2,
      normal
    };

    enum SBy {
      t_00_5ms = 0,
      t_62_5ms,
      t_125ms,
      t_250ms,
      t_500ms,
      t_1000ms,
      t_2000ms,
      t_4000ms,
    };

    // Specify BMP280 configuration
    uint8_t Posr = P_OSR_16, Tosr = T_OSR_02, Mode = normal, IIRFilter = BW0_042ODR, SBy = t_62_5ms;     // set pressure amd temperature output data rate
    // t_fine carries fine temperature as global value for BMP280
    int32_t t_fine;
    //
    // Specify sensor full scale
    uint8_t Gscale = GFS_250DPS;
    uint8_t Ascale = AFS_2G;
    uint8_t Mscale = MFS_16BITS; // Choose either 14-bit or 16-bit magnetometer resolution
    uint8_t Mmode = 0x02;        // 2 for 8 Hz, 6 for 100 Hz continuous magnetometer data read
    float aRes, gRes, mRes;      // scale resolutions per LSB for the sensors

    // Pin definitions
    int intPin = 8;  // These can be changed, 2 and 3 are the Arduinos ext int pins
    int myLed     = 13;  // LED on the Teensy 3.1

    // BMP280 compensation parameters
    uint16_t dig_T1, dig_P1;
    int16_t  dig_T2, dig_T3, dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
    double Temperature, Pressure; // stores BMP280 pressures sensor pressure and temperature
    int32_t rawPress, rawTemp;   // pressure and temperature raw count output for BMP280

    // MPU9250 variables
    int16_t accelCount[3];  // Stores the 16-bit signed accelerometer sensor output
    int16_t gyroCount[3];   // Stores the 16-bit signed gyro sensor output
    int16_t magCount[3];    // Stores the 16-bit signed magnetometer sensor output
    float Quat[4] = {0, 0, 0, 0}; // quaternion data register
    float magCalibration[3] = {0, 0, 0};  // Factory mag calibration and mag bias
    float gyroBias[3] = {0, 0, 0}, accelBias[3] = {0, 0, 0}, magBias[3] = {0, 0, 0}, magScale[3]  = {0, 0, 0};  // Bias corrections for gyro, accelerometer, mag
    int16_t tempCount, rawPressure, rawTemperature;   // pressure, temperature raw count output
    float   temperature, pressure, altitude; // Stores the MPU9250 internal chip temperature in degrees Celsius
    float SelfTest[6];            // holds results of gyro and accelerometer self test

    // global constants for 9 DoF fusion and AHRS (Attitude and Heading Reference System)
    float GyroMeasError = PI * (40.0f / 180.0f);   // gyroscope measurement error in rads/s (start at 40 deg/s)
    float GyroMeasDrift = PI * (0.0f  / 180.0f);   // gyroscope measurement drift in rad/s/s (start at 0.0 deg/s/s)
    // There is a tradeoff in the beta parameter between accuracy and response speed.
    // In the original Madgwick study, beta of 0.041 (corresponding to GyroMeasError of 2.7 degrees/s) was found to give optimal accuracy.
    // However, with this value, the LSM9SD0 response time is about 10 seconds to a stable initial quaternion.
    // Subsequent changes also require a longish lag time to a stable output, not fast enough for a quadcopter or robot car!
    // By increasing beta (GyroMeasError) by about a factor of fifteen, the response time constant is reduced to ~2 sec
    // I haven't noticed any reduction in solution accuracy. This is essentially the I coefficient in a PID control sense;
    // the bigger the feedback coefficient, the faster the solution converges, usually at the expense of accuracy.
    // In any case, this is the free parameter in the Madgwick filtering and fusion scheme.
    float beta = sqrt(3.0f / 4.0f) * GyroMeasError;   // compute beta
    float zeta = sqrt(3.0f / 4.0f) * GyroMeasDrift;   // compute zeta, the other free parameter in the Madgwick scheme usually set to a small or zero value
#define Kp 2.0f * 5.0f // these are the free parameters in the Mahony filter and fusion scheme, Kp for proportional feedback, Ki for integral
#define Ki 0.0f

    uint32_t delt_t = 0, count = 0, sumCount = 0;  // used to control display output rate
    float pitch, yaw, roll, Yaw, Pitch, Roll;
    float deltat = 0.0f, sum = 0.0f;          // integration interval for both filter schemes
    uint32_t lastUpdate = 0, firstUpdate = 0; // used to calculate integration interval
    uint32_t Now = 0;                         // used to calculate integration interval
    uint8_t param[4];                         // used for param transfer
    uint16_t EM7180_mag_fs, EM7180_acc_fs, EM7180_gyro_fs; // EM7180 sensor full scale ranges

    float ax, ay, az, gx, gy, gz, mx, my, mz; // variables to hold latest sensor data values
    float q[4] = {1.0f, 0.0f, 0.0f, 0.0f};    // vector to hold quaternion
    float eInt[3] = {0.0f, 0.0f, 0.0f};       // vector to hold integral error for Mahony method

    bool passThru = false;


    //===================================================================================================================
    //====== Set of useful function to access acceleration. gyroscope, magnetometer, and temperature data
    //===================================================================================================================

    float uint32_reg_to_float (uint8_t *buf)
    {
      union {
        uint32_t ui32;
        float f;
      } u;

      u.ui32 =     (((uint32_t)buf[0]) +
                    (((uint32_t)buf[1]) <<  8) +
                    (((uint32_t)buf[2]) << 16) +
                    (((uint32_t)buf[3]) << 24));
      return u.f;
    }

    void float_to_bytes (float param_val, uint8_t *buf) {
      union {
        float f;
        uint8_t comp[sizeof(float)];
      } u;
      u.f = param_val;
      for (uint8_t i = 0; i < sizeof(float); i++) {
        buf[i] = u.comp[i];
      }
      //Convert to LITTLE ENDIAN
      for (uint8_t i = 0; i < sizeof(float); i++) {
        buf[i] = buf[(sizeof(float) - 1) - i];
      }
    }

    void EM7180_set_gyro_FS (uint16_t gyro_fs) {
      uint8_t bytes[4], STAT;
      bytes[0] = gyro_fs & (0xFF);
      bytes[1] = (gyro_fs >> 8) & (0xFF);
      bytes[2] = 0x00;
      bytes[3] = 0x00;
      writeByte(EM7180_ADDRESS, EM7180_LoadParamByte0, bytes[0]); //Gyro LSB
      writeByte(EM7180_ADDRESS, EM7180_LoadParamByte1, bytes[1]); //Gyro MSB
      writeByte(EM7180_ADDRESS, EM7180_LoadParamByte2, bytes[2]); //Unused
      writeByte(EM7180_ADDRESS, EM7180_LoadParamByte3, bytes[3]); //Unused
      writeByte(EM7180_ADDRESS, EM7180_ParamRequest, 0xCB); //Parameter 75; 0xCB is 75 decimal with the MSB set high to indicate a paramter write processs
      writeByte(EM7180_ADDRESS, EM7180_AlgorithmControl, 0x80); //Request parameter transfer procedure
      STAT = readByte(EM7180_ADDRESS, EM7180_ParamAcknowledge); //Check the parameter acknowledge register and loop until the result matches parameter request byte
      while (!(STAT == 0xCB)) {
        STAT = readByte(EM7180_ADDRESS, EM7180_ParamAcknowledge);
      }
      writeByte(EM7180_ADDRESS, EM7180_ParamRequest, 0x00); //Parameter request = 0 to end parameter transfer process
      writeByte(EM7180_ADDRESS, EM7180_AlgorithmControl, 0x00); // Re-start algorithm
    }

    void EM7180_set_mag_acc_FS (uint16_t mag_fs, uint16_t acc_fs) {
      uint8_t bytes[4], STAT;
      bytes[0] = mag_fs & (0xFF);
      bytes[1] = (mag_fs >> 8) & (0xFF);
      bytes[2] = acc_fs & (0xFF);
      bytes[3] = (acc_fs >> 8) & (0xFF);
      writeByte(EM7180_ADDRESS, EM7180_LoadParamByte0, bytes[0]); //Mag LSB
      writeByte(EM7180_ADDRESS, EM7180_LoadParamByte1, bytes[1]); //Mag MSB
      writeByte(EM7180_ADDRESS, EM7180_LoadParamByte2, bytes[2]); //Acc LSB
      writeByte(EM7180_ADDRESS, EM7180_LoadParamByte3, bytes[3]); //Acc MSB
      writeByte(EM7180_ADDRESS, EM7180_ParamRequest, 0xCA); //Parameter 74; 0xCA is 74 decimal with the MSB set high to indicate a paramter write processs
      writeByte(EM7180_ADDRESS, EM7180_AlgorithmControl, 0x80); //Request parameter transfer procedure
      STAT = readByte(EM7180_ADDRESS, EM7180_ParamAcknowledge); //Check the parameter acknowledge register and loop until the result matches parameter request byte
      while (!(STAT == 0xCA)) {
        STAT = readByte(EM7180_ADDRESS, EM7180_ParamAcknowledge);
      }
      writeByte(EM7180_ADDRESS, EM7180_ParamRequest, 0x00); //Parameter request = 0 to end parameter transfer process
      writeByte(EM7180_ADDRESS, EM7180_AlgorithmControl, 0x00); // Re-start algorithm
    }

    void EM7180_set_integer_param (uint8_t param, uint32_t param_val) {
      uint8_t bytes[4], STAT;
      bytes[0] = param_val & (0xFF);
      bytes[1] = (param_val >> 8) & (0xFF);
      bytes[2] = (param_val >> 16) & (0xFF);
      bytes[3] = (param_val >> 24) & (0xFF);
      param = param | 0x80; //Parameter is the decimal value with the MSB set high to indicate a paramter write processs
      writeByte(EM7180_ADDRESS, EM7180_LoadParamByte0, bytes[0]); //Param LSB
      writeByte(EM7180_ADDRESS, EM7180_LoadParamByte1, bytes[1]);
      writeByte(EM7180_ADDRESS, EM7180_LoadParamByte2, bytes[2]);
      writeByte(EM7180_ADDRESS, EM7180_LoadParamByte3, bytes[3]); //Param MSB
      writeByte(EM7180_ADDRESS, EM7180_ParamRequest, param);
      writeByte(EM7180_ADDRESS, EM7180_AlgorithmControl, 0x80); //Request parameter transfer procedure
      STAT = readByte(EM7180_ADDRESS, EM7180_ParamAcknowledge); //Check the parameter acknowledge register and loop until the result matches parameter request byte
      while (!(STAT == param)) {
        STAT = readByte(EM7180_ADDRESS, EM7180_ParamAcknowledge);
      }
      writeByte(EM7180_ADDRESS, EM7180_ParamRequest, 0x00); //Parameter request = 0 to end parameter transfer process
      writeByte(EM7180_ADDRESS, EM7180_AlgorithmControl, 0x00); // Re-start algorithm
    }

    void EM7180_set_float_param (uint8_t param, float param_val) {
      uint8_t bytes[4], STAT;
      float_to_bytes (param_val, &bytes[0]);
      param = param | 0x80; //Parameter is the decimal value with the MSB set high to indicate a paramter write processs
      writeByte(EM7180_ADDRESS, EM7180_LoadParamByte0, bytes[0]); //Param LSB
      writeByte(EM7180_ADDRESS, EM7180_LoadParamByte1, bytes[1]);
      writeByte(EM7180_ADDRESS, EM7180_LoadParamByte2, bytes[2]);
      writeByte(EM7180_ADDRESS, EM7180_LoadParamByte3, bytes[3]); //Param MSB
      writeByte(EM7180_ADDRESS, EM7180_ParamRequest, param);
      writeByte(EM7180_ADDRESS, EM7180_AlgorithmControl, 0x80); //Request parameter transfer procedure
      STAT = readByte(EM7180_ADDRESS, EM7180_ParamAcknowledge); //Check the parameter acknowledge register and loop until the result matches parameter request byte
      while (!(STAT == param)) {
        STAT = readByte(EM7180_ADDRESS, EM7180_ParamAcknowledge);
      }
      writeByte(EM7180_ADDRESS, EM7180_ParamRequest, 0x00); //Parameter request = 0 to end parameter transfer process
      writeByte(EM7180_ADDRESS, EM7180_AlgorithmControl, 0x00); // Re-start algorithm
    }

    void readSENtralQuatData(float * destination)
    {
      uint8_t rawData[16];  // x/y/z quaternion register data stored here
      readBytes(EM7180_ADDRESS, EM7180_QX, 16, &rawData[0]);       // Read the sixteen raw data registers into data array
      destination[0] = uint32_reg_to_float (&rawData[0]);
      destination[1] = uint32_reg_to_float (&rawData[4]);
      destination[2] = uint32_reg_to_float (&rawData[8]);
      destination[3] = uint32_reg_to_float (&rawData[12]);  // SENtral stores quats as qx, qy, qz, q0!

    }

    void readSENtralAccelData(int16_t * destination)
    {
      uint8_t rawData[6];  // x/y/z accel register data stored here
      readBytes(EM7180_ADDRESS, EM7180_AX, 6, &rawData[0]);       // Read the six raw data registers into data array
      destination[0] = (int16_t) (((int16_t)rawData[1] << 8) | rawData[0]);  // Turn the MSB and LSB into a signed 16-bit value
      destination[1] = (int16_t) (((int16_t)rawData[3] << 8) | rawData[2]);
      destination[2] = (int16_t) (((int16_t)rawData[5] << 8) | rawData[4]);
    }

    void readSENtralGyroData(int16_t * destination)
    {
      uint8_t rawData[6];  // x/y/z gyro register data stored here
      readBytes(EM7180_ADDRESS, EM7180_GX, 6, &rawData[0]);  // Read the six raw data registers sequentially into data array
      destination[0] = (int16_t) (((int16_t)rawData[1] << 8) | rawData[0]);   // Turn the MSB and LSB into a signed 16-bit value
      destination[1] = (int16_t) (((int16_t)rawData[3] << 8) | rawData[2]);
      destination[2] = (int16_t) (((int16_t)rawData[5] << 8) | rawData[4]);
    }

    void readSENtralMagData(int16_t * destination)
    {
      uint8_t rawData[6];  // x/y/z gyro register data stored here
      readBytes(EM7180_ADDRESS, EM7180_MX, 6, &rawData[0]);  // Read the six raw data registers sequentially into data array
      destination[0] = (int16_t) (((int16_t)rawData[1] << 8) | rawData[0]);   // Turn the MSB and LSB into a signed 16-bit value
      destination[1] = (int16_t) (((int16_t)rawData[3] << 8) | rawData[2]);
      destination[2] = (int16_t) (((int16_t)rawData[5] << 8) | rawData[4]);
    }

    void getMres() {
      switch (Mscale)
      {
        // Possible magnetometer scales (and their register bit settings) are:
        // 14 bit resolution (0) and 16 bit resolution (1)
        case MFS_14BITS:
          mRes = 10.*4912. / 8190.; // Proper scale to return milliGauss
          break;
        case MFS_16BITS:
          mRes = 10.*4912. / 32760.0; // Proper scale to return milliGauss
          break;
      }
    }

    void getGres() {
      switch (Gscale)
      {
        // Possible gyro scales (and their register bit settings) are:
        // 250 DPS (00), 500 DPS (01), 1000 DPS (10), and 2000 DPS  (11).
        // Here's a bit of an algorith to calculate DPS/(ADC tick) based on that 2-bit value:
        case GFS_250DPS:
          gRes = 250.0 / 32768.0;
          break;
        case GFS_500DPS:
          gRes = 500.0 / 32768.0;
          break;
        case GFS_1000DPS:
          gRes = 1000.0 / 32768.0;
          break;
        case GFS_2000DPS:
          gRes = 2000.0 / 32768.0;
          break;
      }
    }

    void getAres() {
      switch (Ascale)
      {
        // Possible accelerometer scales (and their register bit settings) are:
        // 2 Gs (00), 4 Gs (01), 8 Gs (10), and 16 Gs  (11).
        // Here's a bit of an algorith to calculate DPS/(ADC tick) based on that 2-bit value:
        case AFS_2G:
          aRes = 2.0 / 32768.0;
          break;
        case AFS_4G:
          aRes = 4.0 / 32768.0;
          break;
        case AFS_8G:
          aRes = 8.0 / 32768.0;
          break;
        case AFS_16G:
          aRes = 16.0 / 32768.0;
          break;
      }
    }


    void readAccelData(int16_t * destination)
    {
      uint8_t rawData[6];  // x/y/z accel register data stored here
      readBytes(MPU9250_ADDRESS, ACCEL_XOUT_H, 6, &rawData[0]);  // Read the six raw data registers into data array
      destination[0] = ((int16_t)rawData[0] << 8) | rawData[1] ;  // Turn the MSB and LSB into a signed 16-bit value
      destination[1] = ((int16_t)rawData[2] << 8) | rawData[3] ;
      destination[2] = ((int16_t)rawData[4] << 8) | rawData[5] ;
    }


    void readGyroData(int16_t * destination)
    {
      uint8_t rawData[6];  // x/y/z gyro register data stored here
      readBytes(MPU9250_ADDRESS, GYRO_XOUT_H, 6, &rawData[0]);  // Read the six raw data registers sequentially into data array
      destination[0] = ((int16_t)rawData[0] << 8) | rawData[1] ;  // Turn the MSB and LSB into a signed 16-bit value
      destination[1] = ((int16_t)rawData[2] << 8) | rawData[3] ;
      destination[2] = ((int16_t)rawData[4] << 8) | rawData[5] ;
    }

    void readMagData(int16_t * destination)
    {
      uint8_t rawData[7];  // x/y/z gyro register data, ST2 register stored here, must read ST2 at end of data acquisition
      if (readByte(AK8963_ADDRESS, AK8963_ST1) & 0x01) { // wait for magnetometer data ready bit to be set
        readBytes(AK8963_ADDRESS, AK8963_XOUT_L, 7, &rawData[0]);  // Read the six raw data and ST2 registers sequentially into data array
        uint8_t c = rawData[6]; // End data read by reading ST2 register
        if (!(c & 0x08)) { // Check if magnetic sensor overflow set, if not then report data
          destination[0] = ((int16_t)rawData[1] << 8) | rawData[0] ;  // Turn the MSB and LSB into a signed 16-bit value
          destination[1] = ((int16_t)rawData[3] << 8) | rawData[2] ;  // Data stored as little Endian
          destination[2] = ((int16_t)rawData[5] << 8) | rawData[4] ;
        }
      }
    }

    int16_t readTempData()
    {
      uint8_t rawData[2];  // x/y/z gyro register data stored here
      readBytes(MPU9250_ADDRESS, TEMP_OUT_H, 2, &rawData[0]);  // Read the two raw data registers sequentially into data array
      return ((int16_t)rawData[0] << 8) | rawData[1] ;  // Turn the MSB and LSB into a 16-bit value
    }

    void initAK8963(float * destination)
    {
      // First extract the factory calibration for each magnetometer axis
      uint8_t rawData[3];  // x/y/z gyro calibration data stored here
      writeByte(AK8963_ADDRESS, AK8963_CNTL, 0x00); // Power down magnetometer
      delay(20);
      writeByte(AK8963_ADDRESS, AK8963_CNTL, 0x0F); // Enter Fuse ROM access mode
      delay(20);
      readBytes(AK8963_ADDRESS, AK8963_ASAX, 3, &rawData[0]);  // Read the x-, y-, and z-axis calibration values
      destination[0] =  (float)(rawData[0] - 128) / 256. + 1.; // Return x-axis sensitivity adjustment values, etc.
      destination[1] =  (float)(rawData[1] - 128) / 256. + 1.;
      destination[2] =  (float)(rawData[2] - 128) / 256. + 1.;
      writeByte(AK8963_ADDRESS, AK8963_CNTL, 0x00); // Power down magnetometer
      delay(20);
      // Configure the magnetometer for continuous read and highest resolution
      // set Mscale bit 4 to 1 (0) to enable 16 (14) bit resolution in CNTL register,
      // and enable continuous mode data acquisition Mmode (bits [3:0]), 0010 for 8 Hz and 0110 for 100 Hz sample rates
      writeByte(AK8963_ADDRESS, AK8963_CNTL, Mscale << 4 | Mmode); // Set magnetometer data resolution and sample ODR
      delay(20);
    }


    void initMPU9250()
    {
      // wake up device
      writeByte(MPU9250_ADDRESS, PWR_MGMT_1, 0x00); // Clear sleep mode bit (6), enable all sensors
      delay(100); // Wait for all registers to reset

      // get stable time source
      writeByte(MPU9250_ADDRESS, PWR_MGMT_1, 0x01);  // Auto select clock source to be PLL gyroscope reference if ready else
      delay(200);

      // Configure Gyro and Thermometer
      // Disable FSYNC and set thermometer and gyro bandwidth to 41 and 42 Hz, respectively;
      // minimum delay time for this setting is 5.9 ms, which means sensor fusion update rates cannot
      // be higher than 1 / 0.0059 = 170 Hz
      // DLPF_CFG = bits 2:0 = 011; this limits the sample rate to 1000 Hz for both
      // With the MPU9250, it is possible to get gyro sample rates of 32 kHz (!), 8 kHz, or 1 kHz
      writeByte(MPU9250_ADDRESS, CONFIG, 0x03);

      // Set sample rate = gyroscope output rate/(1 + SMPLRT_DIV)
      writeByte(MPU9250_ADDRESS, SMPLRT_DIV, 0x04);  // Use a 200 Hz rate; a rate consistent with the filter update rate
      // determined inset in CONFIG above

      // Set gyroscope full scale range
      // Range selects FS_SEL and AFS_SEL are 0 - 3, so 2-bit values are left-shifted into positions 4:3
      uint8_t c = readByte(MPU9250_ADDRESS, GYRO_CONFIG); // get current GYRO_CONFIG register value
      // c = c & ~0xE0; // Clear self-test bits [7:5]
      c = c & ~0x02; // Clear Fchoice bits [1:0]
      c = c & ~0x18; // Clear AFS bits [4:3]
      c = c | Gscale << 3; // Set full scale range for the gyro
      // c =| 0x00; // Set Fchoice for the gyro to 11 by writing its inverse to bits 1:0 of GYRO_CONFIG
      writeByte(MPU9250_ADDRESS, GYRO_CONFIG, c ); // Write new GYRO_CONFIG value to register

      // Set accelerometer full-scale range configuration
      c = readByte(MPU9250_ADDRESS, ACCEL_CONFIG); // get current ACCEL_CONFIG register value
      // c = c & ~0xE0; // Clear self-test bits [7:5]
      c = c & ~0x18;  // Clear AFS bits [4:3]
      c = c | Ascale << 3; // Set full scale range for the accelerometer
      writeByte(MPU9250_ADDRESS, ACCEL_CONFIG, c); // Write new ACCEL_CONFIG register value

      // Set accelerometer sample rate configuration
      // It is possible to get a 4 kHz sample rate from the accelerometer by choosing 1 for
      // accel_fchoice_b bit [3]; in this case the bandwidth is 1.13 kHz
      c = readByte(MPU9250_ADDRESS, ACCEL_CONFIG2); // get current ACCEL_CONFIG2 register value
      c = c & ~0x0F; // Clear accel_fchoice_b (bit 3) and A_DLPFG (bits [2:0])
      c = c | 0x03;  // Set accelerometer rate to 1 kHz and bandwidth to 41 Hz
      writeByte(MPU9250_ADDRESS, ACCEL_CONFIG2, c); // Write new ACCEL_CONFIG2 register value

      // The accelerometer, gyro, and thermometer are set to 1 kHz sample rates,
      // but all these rates are further reduced by a factor of 5 to 200 Hz because of the SMPLRT_DIV setting

      // Configure Interrupts and Bypass Enable
      // Set interrupt pin active high, push-pull, hold interrupt pin level HIGH until interrupt cleared,
      // clear on read of INT_STATUS, and enable I2C_BYPASS_EN so additional chips
      // can join the I2C bus and all can be controlled by the Arduino as master
      writeByte(MPU9250_ADDRESS, INT_PIN_CFG, 0x22);
      writeByte(MPU9250_ADDRESS, INT_ENABLE, 0x01);  // Enable data ready (bit 0) interrupt
      delay(100);
    }


    // Function which accumulates gyro and accelerometer data after device initialization. It calculates the average
    // of the at-rest readings and then loads the resulting offsets into accelerometer and gyro bias registers.
    void accelgyrocalMPU9250(float * dest1, float * dest2)
    {
      uint8_t data[12]; // data array to hold accelerometer and gyro x, y, z, data
      uint16_t ii, packet_count, fifo_count;
      int32_t gyro_bias[3]  = {0, 0, 0}, accel_bias[3] = {0, 0, 0};

      // reset device
      writeByte(MPU9250_ADDRESS, PWR_MGMT_1, 0x80); // Write a one to bit 7 reset bit; toggle reset device
      delay(100);

      // get stable time source; Auto select clock source to be PLL gyroscope reference if ready
      // else use the internal oscillator, bits 2:0 = 001
      writeByte(MPU9250_ADDRESS, PWR_MGMT_1, 0x01);
      writeByte(MPU9250_ADDRESS, PWR_MGMT_2, 0x00);
      delay(200);

      // Configure device for bias calculation
      writeByte(MPU9250_ADDRESS, INT_ENABLE, 0x00);   // Disable all interrupts
      writeByte(MPU9250_ADDRESS, FIFO_EN, 0x00);      // Disable FIFO
      writeByte(MPU9250_ADDRESS, PWR_MGMT_1, 0x00);   // Turn on internal clock source
      writeByte(MPU9250_ADDRESS, I2C_MST_CTRL, 0x00); // Disable I2C master
      writeByte(MPU9250_ADDRESS, USER_CTRL, 0x00);    // Disable FIFO and I2C master modes
      writeByte(MPU9250_ADDRESS, USER_CTRL, 0x0C);    // Reset FIFO and DMP
      delay(15);

      // Configure MPU6050 gyro and accelerometer for bias calculation
      writeByte(MPU9250_ADDRESS, CONFIG, 0x01);      // Set low-pass filter to 188 Hz
      writeByte(MPU9250_ADDRESS, SMPLRT_DIV, 0x00);  // Set sample rate to 1 kHz
      writeByte(MPU9250_ADDRESS, GYRO_CONFIG, 0x00);  // Set gyro full-scale to 250 degrees per second, maximum sensitivity
      writeByte(MPU9250_ADDRESS, ACCEL_CONFIG, 0x00); // Set accelerometer full-scale to 2 g, maximum sensitivity

      uint16_t  gyrosensitivity  = 131;   // = 131 LSB/degrees/sec
      uint16_t  accelsensitivity = 16384;  // = 16384 LSB/g

      // Configure FIFO to capture accelerometer and gyro data for bias calculation
      writeByte(MPU9250_ADDRESS, USER_CTRL, 0x40);   // Enable FIFO
      writeByte(MPU9250_ADDRESS, FIFO_EN, 0x78);     // Enable gyro and accelerometer sensors for FIFO  (max size 512 bytes in MPU-9150)
      delay(40); // accumulate 40 samples in 40 milliseconds = 480 bytes

      // At end of sample accumulation, turn off FIFO sensor read
      writeByte(MPU9250_ADDRESS, FIFO_EN, 0x00);        // Disable gyro and accelerometer sensors for FIFO
      readBytes(MPU9250_ADDRESS, FIFO_COUNTH, 2, &data[0]); // read FIFO sample count
      fifo_count = ((uint16_t)data[0] << 8) | data[1];
      packet_count = fifo_count / 12; // How many sets of full gyro and accelerometer data for averaging

      for (ii = 0; ii < packet_count; ii++) {
        int16_t accel_temp[3] = {0, 0, 0}, gyro_temp[3] = {0, 0, 0};
        readBytes(MPU9250_ADDRESS, FIFO_R_W, 12, &data[0]); // read data for averaging
        accel_temp[0] = (int16_t) (((int16_t)data[0] << 8) | data[1]  ) ;  // Form signed 16-bit integer for each sample in FIFO
        accel_temp[1] = (int16_t) (((int16_t)data[2] << 8) | data[3]  ) ;
        accel_temp[2] = (int16_t) (((int16_t)data[4] << 8) | data[5]  ) ;
        gyro_temp[0]  = (int16_t) (((int16_t)data[6] << 8) | data[7]  ) ;
        gyro_temp[1]  = (int16_t) (((int16_t)data[8] << 8) | data[9]  ) ;
        gyro_temp[2]  = (int16_t) (((int16_t)data[10] << 8) | data[11]) ;

        accel_bias[0] += (int32_t) accel_temp[0]; // Sum individual signed 16-bit biases to get accumulated signed 32-bit biases
        accel_bias[1] += (int32_t) accel_temp[1];
        accel_bias[2] += (int32_t) accel_temp[2];
        gyro_bias[0]  += (int32_t) gyro_temp[0];
        gyro_bias[1]  += (int32_t) gyro_temp[1];
        gyro_bias[2]  += (int32_t) gyro_temp[2];

      }
      accel_bias[0] /= (int32_t) packet_count; // Normalize sums to get average count biases
      accel_bias[1] /= (int32_t) packet_count;
      accel_bias[2] /= (int32_t) packet_count;
      gyro_bias[0]  /= (int32_t) packet_count;
      gyro_bias[1]  /= (int32_t) packet_count;
      gyro_bias[2]  /= (int32_t) packet_count;

      if (accel_bias[2] > 0L) {
        accel_bias[2] -= (int32_t) accelsensitivity; // Remove gravity from the z-axis accelerometer bias calculation
      }
      else {
        accel_bias[2] += (int32_t) accelsensitivity;
      }

      // Construct the gyro biases for push to the hardware gyro bias registers, which are reset to zero upon device startup
      data[0] = (-gyro_bias[0] / 4  >> 8) & 0xFF; // Divide by 4 to get 32.9 LSB per deg/s to conform to expected bias input format
      data[1] = (-gyro_bias[0] / 4)       & 0xFF; // Biases are additive, so change sign on calculated average gyro biases
      data[2] = (-gyro_bias[1] / 4  >> 8) & 0xFF;
      data[3] = (-gyro_bias[1] / 4)       & 0xFF;
      data[4] = (-gyro_bias[2] / 4  >> 8) & 0xFF;
      data[5] = (-gyro_bias[2] / 4)       & 0xFF;

      // Push gyro biases to hardware registers
      writeByte(MPU9250_ADDRESS, XG_OFFSET_H, data[0]);
      writeByte(MPU9250_ADDRESS, XG_OFFSET_L, data[1]);
      writeByte(MPU9250_ADDRESS, YG_OFFSET_H, data[2]);
      writeByte(MPU9250_ADDRESS, YG_OFFSET_L, data[3]);
      writeByte(MPU9250_ADDRESS, ZG_OFFSET_H, data[4]);
      writeByte(MPU9250_ADDRESS, ZG_OFFSET_L, data[5]);

      // Output scaled gyro biases for display in the main program
      dest1[0] = (float) gyro_bias[0] / (float) gyrosensitivity;
      dest1[1] = (float) gyro_bias[1] / (float) gyrosensitivity;
      dest1[2] = (float) gyro_bias[2] / (float) gyrosensitivity;

      // Construct the accelerometer biases for push to the hardware accelerometer bias registers. These registers contain
      // factory trim values which must be added to the calculated accelerometer biases; on boot up these registers will hold
      // non-zero values. In addition, bit 0 of the lower byte must be preserved since it is used for temperature
      // compensation calculations. Accelerometer bias registers expect bias input as 2048 LSB per g, so that
      // the accelerometer biases calculated above must be divided by 8.

      int32_t accel_bias_reg[3] = {0, 0, 0}; // A place to hold the factory accelerometer trim biases
      readBytes(MPU9250_ADDRESS, XA_OFFSET_H, 2, &data[0]); // Read factory accelerometer trim values
      accel_bias_reg[0] = (int32_t) (((int16_t)data[0] << 8) | data[1]);
      readBytes(MPU9250_ADDRESS, YA_OFFSET_H, 2, &data[0]);
      accel_bias_reg[1] = (int32_t) (((int16_t)data[0] << 8) | data[1]);
      readBytes(MPU9250_ADDRESS, ZA_OFFSET_H, 2, &data[0]);
      accel_bias_reg[2] = (int32_t) (((int16_t)data[0] << 8) | data[1]);

      uint32_t mask = 1uL; // Define mask for temperature compensation bit 0 of lower byte of accelerometer bias registers
      uint8_t mask_bit[3] = {0, 0, 0}; // Define array to hold mask bit for each accelerometer bias axis

      for (ii = 0; ii < 3; ii++) {
        if ((accel_bias_reg[ii] & mask)) mask_bit[ii] = 0x01; // If temperature compensation bit is set, record that fact in mask_bit
      }

      // Construct total accelerometer bias, including calculated average accelerometer bias from above
      accel_bias_reg[0] -= (accel_bias[0] / 8); // Subtract calculated averaged accelerometer bias scaled to 2048 LSB/g (16 g full scale)
      accel_bias_reg[1] -= (accel_bias[1] / 8);
      accel_bias_reg[2] -= (accel_bias[2] / 8);

      data[0] = (accel_bias_reg[0] >> 8) & 0xFE;
      data[1] = (accel_bias_reg[0])      & 0xFE;
      data[1] = data[1] | mask_bit[0]; // preserve temperature compensation bit when writing back to accelerometer bias registers
      data[2] = (accel_bias_reg[1] >> 8) & 0xFE;
      data[3] = (accel_bias_reg[1])      & 0xFE;
      data[3] = data[3] | mask_bit[1]; // preserve temperature compensation bit when writing back to accelerometer bias registers
      data[4] = (accel_bias_reg[2] >> 8) & 0xFE;
      data[5] = (accel_bias_reg[2])      & 0xFE;
      data[5] = data[5] | mask_bit[2]; // preserve temperature compensation bit when writing back to accelerometer bias registers

      // Apparently this is not working for the acceleration biases in the MPU-9250
      // Are we handling the temperature correction bit properly?
      // Push accelerometer biases to hardware registers
      /*  writeByte(MPU9250_ADDRESS, XA_OFFSET_H, data[0]);
        writeByte(MPU9250_ADDRESS, XA_OFFSET_L, data[1]);
        writeByte(MPU9250_ADDRESS, YA_OFFSET_H, data[2]);
        writeByte(MPU9250_ADDRESS, YA_OFFSET_L, data[3]);
        writeByte(MPU9250_ADDRESS, ZA_OFFSET_H, data[4]);
        writeByte(MPU9250_ADDRESS, ZA_OFFSET_L, data[5]);
      */
      // Output scaled accelerometer biases for display in the main program
      dest2[0] = (float)accel_bias[0] / (float)accelsensitivity;
      dest2[1] = (float)accel_bias[1] / (float)accelsensitivity;
      dest2[2] = (float)accel_bias[2] / (float)accelsensitivity;
    }


    void magcalMPU9250(float * dest1, float * dest2)
    {
      uint16_t ii = 0, sample_count = 0;
      int32_t mag_bias[3] = {0, 0, 0}, mag_scale[3] = {0, 0, 0};
      int16_t mag_max[3] = {0xFF, 0xFF, 0xFF}, mag_min[3] = {0x7F, 0x7F, 0x7F}, mag_temp[3] = {0, 0, 0};

      Serial.println("Mag Calibration: Wave device in a figure eight until done!");
      delay(4000);

      if (Mmode == 0x02) sample_count = 128;
      if (Mmode == 0x06) sample_count = 1500;
      for (ii = 0; ii < sample_count; ii++) {
        readMagData(mag_temp);  // Read the mag data
        for (int jj = 0; jj < 3; jj++) {
          if (mag_temp[jj] > mag_max[jj]) mag_max[jj] = mag_temp[jj];
          if (mag_temp[jj] < mag_min[jj]) mag_min[jj] = mag_temp[jj];
        }
        if (Mmode == 0x02) delay(135); // at 8 Hz ODR, new mag data is available every 125 ms
        if (Mmode == 0x06) delay(12); // at 100 Hz ODR, new mag data is available every 10 ms
      }

      //    Serial.println("mag x min/max:"); Serial.println(mag_max[0]); Serial.println(mag_min[0]);
      //    Serial.println("mag y min/max:"); Serial.println(mag_max[1]); Serial.println(mag_min[1]);
      //    Serial.println("mag z min/max:"); Serial.println(mag_max[2]); Serial.println(mag_min[2]);

      // Get hard iron correction
      mag_bias[0]  = (mag_max[0] + mag_min[0]) / 2; // get average x mag bias in counts
      mag_bias[1]  = (mag_max[1] + mag_min[1]) / 2; // get average y mag bias in counts
      mag_bias[2]  = (mag_max[2] + mag_min[2]) / 2; // get average z mag bias in counts

      dest1[0] = (float) mag_bias[0] * mRes * magCalibration[0]; // save mag biases in G for main program
      dest1[1] = (float) mag_bias[1] * mRes * magCalibration[1];
      dest1[2] = (float) mag_bias[2] * mRes * magCalibration[2];

      // Get soft iron correction estimate
      mag_scale[0]  = (mag_max[0] - mag_min[0]) / 2; // get average x axis max chord length in counts
      mag_scale[1]  = (mag_max[1] - mag_min[1]) / 2; // get average y axis max chord length in counts
      mag_scale[2]  = (mag_max[2] - mag_min[2]) / 2; // get average z axis max chord length in counts

      float avg_rad = mag_scale[0] + mag_scale[1] + mag_scale[2];
      avg_rad /= 3.0;

      dest2[0] = avg_rad / ((float)mag_scale[0]);
      dest2[1] = avg_rad / ((float)mag_scale[1]);
      dest2[2] = avg_rad / ((float)mag_scale[2]);

      Serial.println("Mag Calibration done!");
    }




    // Accelerometer and gyroscope self test; check calibration wrt factory settings
    void MPU9250SelfTest(float * destination) // Should return percent deviation from factory trim values, +/- 14 or less deviation is a pass
    {
      uint8_t rawData[6] = {0, 0, 0, 0, 0, 0};
      uint8_t selfTest[6];
      int16_t gAvg[3], aAvg[3], aSTAvg[3], gSTAvg[3];
      float factoryTrim[6];
      uint8_t FS = 0;

      writeByte(MPU9250_ADDRESS, SMPLRT_DIV, 0x00);    // Set gyro sample rate to 1 kHz
      writeByte(MPU9250_ADDRESS, CONFIG, 0x02);        // Set gyro sample rate to 1 kHz and DLPF to 92 Hz
      writeByte(MPU9250_ADDRESS, GYRO_CONFIG, FS << 3); // Set full scale range for the gyro to 250 dps
      writeByte(MPU9250_ADDRESS, ACCEL_CONFIG2, 0x02); // Set accelerometer rate to 1 kHz and bandwidth to 92 Hz
      writeByte(MPU9250_ADDRESS, ACCEL_CONFIG, FS << 3); // Set full scale range for the accelerometer to 2 g

      for ( int ii = 0; ii < 200; ii++) { // get average current values of gyro and acclerometer

        readBytes(MPU9250_ADDRESS, ACCEL_XOUT_H, 6, &rawData[0]);        // Read the six raw data registers into data array
        aAvg[0] += (int16_t)(((int16_t)rawData[0] << 8) | rawData[1]) ;  // Turn the MSB and LSB into a signed 16-bit value
        aAvg[1] += (int16_t)(((int16_t)rawData[2] << 8) | rawData[3]) ;
        aAvg[2] += (int16_t)(((int16_t)rawData[4] << 8) | rawData[5]) ;

        readBytes(MPU9250_ADDRESS, GYRO_XOUT_H, 6, &rawData[0]);       // Read the six raw data registers sequentially into data array
        gAvg[0] += (int16_t)(((int16_t)rawData[0] << 8) | rawData[1]) ;  // Turn the MSB and LSB into a signed 16-bit value
        gAvg[1] += (int16_t)(((int16_t)rawData[2] << 8) | rawData[3]) ;
        gAvg[2] += (int16_t)(((int16_t)rawData[4] << 8) | rawData[5]) ;
      }

      for (int ii = 0; ii < 3; ii++) { // Get average of 200 values and store as average current readings
        aAvg[ii] /= 200;
        gAvg[ii] /= 200;
      }

      // Configure the accelerometer for self-test
      writeByte(MPU9250_ADDRESS, ACCEL_CONFIG, 0xE0); // Enable self test on all three axes and set accelerometer range to +/- 2 g
      writeByte(MPU9250_ADDRESS, GYRO_CONFIG,  0xE0); // Enable self test on all three axes and set gyro range to +/- 250 degrees/s
      delay(25);  // Delay a while to let the device stabilize

      for ( int ii = 0; ii < 200; ii++) { // get average self-test values of gyro and acclerometer

        readBytes(MPU9250_ADDRESS, ACCEL_XOUT_H, 6, &rawData[0]);  // Read the six raw data registers into data array
        aSTAvg[0] += (int16_t)(((int16_t)rawData[0] << 8) | rawData[1]) ;  // Turn the MSB and LSB into a signed 16-bit value
        aSTAvg[1] += (int16_t)(((int16_t)rawData[2] << 8) | rawData[3]) ;
        aSTAvg[2] += (int16_t)(((int16_t)rawData[4] << 8) | rawData[5]) ;

        readBytes(MPU9250_ADDRESS, GYRO_XOUT_H, 6, &rawData[0]);  // Read the six raw data registers sequentially into data array
        gSTAvg[0] += (int16_t)(((int16_t)rawData[0] << 8) | rawData[1]) ;  // Turn the MSB and LSB into a signed 16-bit value
        gSTAvg[1] += (int16_t)(((int16_t)rawData[2] << 8) | rawData[3]) ;
        gSTAvg[2] += (int16_t)(((int16_t)rawData[4] << 8) | rawData[5]) ;
      }

      for (int ii = 0; ii < 3; ii++) { // Get average of 200 values and store as average self-test readings
        aSTAvg[ii] /= 200;
        gSTAvg[ii] /= 200;
      }

      // Configure the gyro and accelerometer for normal operation
      writeByte(MPU9250_ADDRESS, ACCEL_CONFIG, 0x00);
      writeByte(MPU9250_ADDRESS, GYRO_CONFIG,  0x00);
      delay(25);  // Delay a while to let the device stabilize

      // Retrieve accelerometer and gyro factory Self-Test Code from USR_Reg
      selfTest[0] = readByte(MPU9250_ADDRESS, SELF_TEST_X_ACCEL); // X-axis accel self-test results
      selfTest[1] = readByte(MPU9250_ADDRESS, SELF_TEST_Y_ACCEL); // Y-axis accel self-test results
      selfTest[2] = readByte(MPU9250_ADDRESS, SELF_TEST_Z_ACCEL); // Z-axis accel self-test results
      selfTest[3] = readByte(MPU9250_ADDRESS, SELF_TEST_X_GYRO);  // X-axis gyro self-test results
      selfTest[4] = readByte(MPU9250_ADDRESS, SELF_TEST_Y_GYRO);  // Y-axis gyro self-test results
      selfTest[5] = readByte(MPU9250_ADDRESS, SELF_TEST_Z_GYRO);  // Z-axis gyro self-test results

      // Retrieve factory self-test value from self-test code reads
      factoryTrim[0] = (float)(2620 / 1 << FS) * (pow( 1.01 , ((float)selfTest[0] - 1.0) )); // FT[Xa] factory trim calculation
      factoryTrim[1] = (float)(2620 / 1 << FS) * (pow( 1.01 , ((float)selfTest[1] - 1.0) )); // FT[Ya] factory trim calculation
      factoryTrim[2] = (float)(2620 / 1 << FS) * (pow( 1.01 , ((float)selfTest[2] - 1.0) )); // FT[Za] factory trim calculation
      factoryTrim[3] = (float)(2620 / 1 << FS) * (pow( 1.01 , ((float)selfTest[3] - 1.0) )); // FT[Xg] factory trim calculation
      factoryTrim[4] = (float)(2620 / 1 << FS) * (pow( 1.01 , ((float)selfTest[4] - 1.0) )); // FT[Yg] factory trim calculation
      factoryTrim[5] = (float)(2620 / 1 << FS) * (pow( 1.01 , ((float)selfTest[5] - 1.0) )); // FT[Zg] factory trim calculation

      // Report results as a ratio of (STR - FT)/FT; the change from Factory Trim of the Self-Test Response
      // To get percent, must multiply by 100
      for (int i = 0; i < 3; i++) {
        destination[i]   = 100.0 * ((float)(aSTAvg[i] - aAvg[i])) / factoryTrim[i]; // Report percent differences
        destination[i + 3] = 100.0 * ((float)(gSTAvg[i] - gAvg[i])) / factoryTrim[i + 3]; // Report percent differences
      }

    }

    int16_t readSENtralBaroData()
    {
      uint8_t rawData[2];  // x/y/z gyro register data stored here
      readBytes(EM7180_ADDRESS, EM7180_Baro, 2, &rawData[0]);  // Read the two raw data registers sequentially into data array
      return  (int16_t) (((int16_t)rawData[1] << 8) | rawData[0]);   // Turn the MSB and LSB into a signed 16-bit value
    }

    int16_t readSENtralTempData()
    {
      uint8_t rawData[2];  // x/y/z gyro register data stored here
      readBytes(EM7180_ADDRESS, EM7180_Temp, 2, &rawData[0]);  // Read the two raw data registers sequentially into data array
      return  (int16_t) (((int16_t)rawData[1] << 8) | rawData[0]);   // Turn the MSB and LSB into a signed 16-bit value
    }


    void SENtralPassThroughMode()
    {
      // First put SENtral in standby mode
      uint8_t c = readByte(EM7180_ADDRESS, EM7180_AlgorithmControl);
      writeByte(EM7180_ADDRESS, EM7180_AlgorithmControl, c | 0x01);
      //  c = readByte(EM7180_ADDRESS, EM7180_AlgorithmStatus);
      //  Serial.print("c = "); Serial.println(c);
      // Verify standby status
      // if(readByte(EM7180_ADDRESS, EM7180_AlgorithmStatus) & 0x01) {
      Serial.println("SENtral in standby mode");
      // Place SENtral in pass-through mode
      writeByte(EM7180_ADDRESS, EM7180_PassThruControl, 0x01);
      if (readByte(EM7180_ADDRESS, EM7180_PassThruStatus) & 0x01) {
        Serial.println("SENtral in pass-through mode");
      }
      else {
        Serial.println("ERROR! SENtral not in pass-through mode!");
      }


    }



    // I2C communication with the M24512DFM EEPROM is a little different from I2C communication with the usual motion sensor
    // since the address is defined by two bytes

    void M24512DFMwriteByte(uint8_t device_address, uint8_t data_address1, uint8_t data_address2, uint8_t  data)
    {
      Wire.beginTransmission(device_address);   // Initialize the Tx buffer
      Wire.write(data_address1);                // Put slave register address in Tx buffer
      Wire.write(data_address2);                // Put slave register address in Tx buffer
      Wire.write(data);                         // Put data in Tx buffer
      Wire.endTransmission();                   // Send the Tx buffer
    }


    void M24512DFMwriteBytes(uint8_t device_address, uint8_t data_address1, uint8_t data_address2, uint8_t count, uint8_t * dest)
    {
      if (count > 128) {
        count = 128;
        Serial.print("Page count cannot be more than 128 bytes!");
      }

      Wire.beginTransmission(device_address);   // Initialize the Tx buffer
      Wire.write(data_address1);                // Put slave register address in Tx buffer
      Wire.write(data_address2);                // Put slave register address in Tx buffer
      for (uint8_t i = 0; i < count; i++) {
        Wire.write(dest[i]);                      // Put data in Tx buffer
      }
      Wire.endTransmission();                   // Send the Tx buffer
    }


    uint8_t M24512DFMreadByte(uint8_t device_address, uint8_t data_address1, uint8_t data_address2)
    {
      uint8_t data; // `data` will store the register data
      Wire.beginTransmission(device_address);         // Initialize the Tx buffer
      Wire.write(data_address1);                // Put slave register address in Tx buffer
      Wire.write(data_address2);                // Put slave register address in Tx buffer
      Wire.endTransmission(I2C_NOSTOP);        // Send the Tx buffer, but send a restart to keep connection alive
      //  Wire.endTransmission(false);             // Send the Tx buffer, but send a restart to keep connection alive
      //  Wire.requestFrom(address, 1);  // Read one byte from slave register address
      Wire.requestFrom(device_address, (size_t) 1);   // Read one byte from slave register address
      data = Wire.read();                      // Fill Rx buffer with result
      return data;                             // Return data read from slave register
    }

    void M24512DFMreadBytes(uint8_t device_address, uint8_t data_address1, uint8_t data_address2, uint8_t count, uint8_t * dest)
    {
      Wire.beginTransmission(device_address);   // Initialize the Tx buffer
      Wire.write(data_address1);                     // Put slave register address in Tx buffer
      Wire.write(data_address2);                     // Put slave register address in Tx buffer
      Wire.endTransmission(I2C_NOSTOP);         // Send the Tx buffer, but send a restart to keep connection alive
      //  Wire.endTransmission(false);              // Send the Tx buffer, but send a restart to keep connection alive
      uint8_t i = 0;
      //        Wire.requestFrom(address, count);       // Read bytes from slave register address
      Wire.requestFrom(device_address, (size_t) count);  // Read bytes from slave register address
      while (Wire.available()) {
        dest[i++] = Wire.read();
      }                // Put read results in the Rx buffer
    }





    int32_t readBMP280Temperature()
    {
      uint8_t rawData[3];  // 20-bit pressure register data stored here
      readBytes(BMP280_ADDRESS, BMP280_TEMP_MSB, 3, &rawData[0]);
      return (int32_t) (((int32_t) rawData[0] << 16 | (int32_t) rawData[1] << 8 | rawData[2]) >> 4);
    }

    int32_t readBMP280Pressure()
    {
      uint8_t rawData[3];  // 20-bit pressure register data stored here
      readBytes(BMP280_ADDRESS, BMP280_PRESS_MSB, 3, &rawData[0]);
      return (int32_t) (((int32_t) rawData[0] << 16 | (int32_t) rawData[1] << 8 | rawData[2]) >> 4);
    }

    void BMP280Init()
    {
      // Configure the BMP280
      // Set T and P oversampling rates and sensor mode
      writeByte(BMP280_ADDRESS, BMP280_CTRL_MEAS, Tosr << 5 | Posr << 2 | Mode);
      // Set standby time interval in normal mode and bandwidth
      writeByte(BMP280_ADDRESS, BMP280_CONFIG, SBy << 5 | IIRFilter << 2);
      // Read and store calibration data
      uint8_t calib[24];
      readBytes(BMP280_ADDRESS, BMP280_CALIB00, 24, &calib[0]);
      dig_T1 = (uint16_t)(((uint16_t) calib[1] << 8) | calib[0]);
      dig_T2 = ( int16_t)((( int16_t) calib[3] << 8) | calib[2]);
      dig_T3 = ( int16_t)((( int16_t) calib[5] << 8) | calib[4]);
      dig_P1 = (uint16_t)(((uint16_t) calib[7] << 8) | calib[6]);
      dig_P2 = ( int16_t)((( int16_t) calib[9] << 8) | calib[8]);
      dig_P3 = ( int16_t)((( int16_t) calib[11] << 8) | calib[10]);
      dig_P4 = ( int16_t)((( int16_t) calib[13] << 8) | calib[12]);
      dig_P5 = ( int16_t)((( int16_t) calib[15] << 8) | calib[14]);
      dig_P6 = ( int16_t)((( int16_t) calib[17] << 8) | calib[16]);
      dig_P7 = ( int16_t)((( int16_t) calib[19] << 8) | calib[18]);
      dig_P8 = ( int16_t)((( int16_t) calib[21] << 8) | calib[20]);
      dig_P9 = ( int16_t)((( int16_t) calib[23] << 8) | calib[22]);
    }

    // Returns temperature in DegC, resolution is 0.01 DegC. Output value of
    // “5123” equals 51.23 DegC.
    int32_t bmp280_compensate_T(int32_t adc_T)
    {
      int32_t var1, var2, T;
      var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
      var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
      t_fine = var1 + var2;
      T = (t_fine * 5 + 128) >> 8;
      return T;
    }

    // Returns pressure in Pa as unsigned 32 bit integer in Q24.8 format (24 integer bits and 8
    //fractional bits).
    //Output value of “24674867” represents 24674867/256 = 96386.2 Pa = 963.862 hPa
    uint32_t bmp280_compensate_P(int32_t adc_P)
    {
      long long var1, var2, p;
      var1 = ((long long)t_fine) - 128000;
      var2 = var1 * var1 * (long long)dig_P6;
      var2 = var2 + ((var1 * (long long)dig_P5) << 17);
      var2 = var2 + (((long long)dig_P4) << 35);
      var1 = ((var1 * var1 * (long long)dig_P3) >> 8) + ((var1 * (long long)dig_P2) << 12);
      var1 = (((((long long)1) << 47) + var1)) * ((long long)dig_P1) >> 33;
      if (var1 == 0)
      {
        return 0;
        // avoid exception caused by division by zero
      }
      p = 1048576 - adc_P;
      p = (((p << 31) - var2) * 3125) / var1;
      var1 = (((long long)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
      var2 = (((long long)dig_P8) * p) >> 19;
      p = ((p + var1 + var2) >> 8) + (((long long)dig_P7) << 4);
      return (uint32_t)p;
    }




    // simple function to scan for I2C devices on the bus
    void I2Cscan()
    {
      // scan for i2c devices
      byte error, address;
      int nDevices;

      Serial.println("Scanning...");

      nDevices = 0;
      for (address = 1; address < 127; address++ )
      {
        // The i2c_scanner uses the return value of
        // the Write.endTransmisstion to see if
        // a device did acknowledge to the address.
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0)
        {
          Serial.print("I2C device found at address 0x");
          if (address < 16)
            Serial.print("0");
          Serial.print(address, HEX);
          Serial.println("  !");

          nDevices++;
        }
        else if (error == 4)
        {
          Serial.print("Unknow error at address 0x");
          if (address < 16)
            Serial.print("0");
          Serial.println(address, HEX);
        }
      }
      if (nDevices == 0)
        Serial.println("No I2C devices found\n");
      else
        Serial.println("done\n");
    }


    // I2C read/write functions for the MPU9250 and AK8963 sensors

    void writeByte(uint8_t address, uint8_t subAddress, uint8_t data)
    {
      Wire.beginTransmission(address);  // Initialize the Tx buffer
      Wire.write(subAddress);           // Put slave register address in Tx buffer
      Wire.write(data);                 // Put data in Tx buffer
      Wire.endTransmission();           // Send the Tx buffer
    }

    uint8_t readByte(uint8_t address, uint8_t subAddress)
    {
      uint8_t data; // `data` will store the register data
      Wire.beginTransmission(address);         // Initialize the Tx buffer
      Wire.write(subAddress);                  // Put slave register address in Tx buffer
      Wire.endTransmission(I2C_NOSTOP);        // Send the Tx buffer, but send a restart to keep connection alive
      //  Wire.endTransmission(false);             // Send the Tx buffer, but send a restart to keep connection alive
      //  Wire.requestFrom(address, 1);  // Read one byte from slave register address
      Wire.requestFrom(address, (size_t) 1);   // Read one byte from slave register address
      data = Wire.read();                      // Fill Rx buffer with result
      return data;                             // Return data read from slave register
    }

    void readBytes(uint8_t address, uint8_t subAddress, uint8_t count, uint8_t * dest)
    {
      Wire.beginTransmission(address);   // Initialize the Tx buffer
      Wire.write(subAddress);            // Put slave register address in Tx buffer
      Wire.endTransmission(I2C_NOSTOP);  // Send the Tx buffer, but send a restart to keep connection alive
      //  Wire.endTransmission(false);       // Send the Tx buffer, but send a restart to keep connection alive
      uint8_t i = 0;
      //        Wire.requestFrom(address, count);  // Read bytes from slave register address
      Wire.requestFrom(address, (size_t) count);  // Read bytes from slave register address
      while (Wire.available()) {
        dest[i++] = Wire.read();
      }         // Put read results in the Rx buffer
    }


    // Implementation of Sebastian Madgwick's "...efficient orientation filter for... inertial/magnetic sensor arrays"
    // (see http://www.x-io.co.uk/category/open-source/ for examples and more details)
    // which fuses acceleration, rotation rate, and magnetic moments to produce a quaternion-based estimate of absolute
    // device orientation -- which can be converted to yaw, pitch, and roll. Useful for stabilizing quadcopters, etc.
    // The performance of the orientation filter is at least as good as conventional Kalman-based filtering algorithms
    // but is much less computationally intensive---it can be performed on a 3.3 V Pro Mini operating at 8 MHz!
    void MadgwickQuaternionUpdate(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz)
    {
      float q1 = q[0], q2 = q[1], q3 = q[2], q4 = q[3];   // short name local variable for readability
      float norm;
      float hx, hy, _2bx, _2bz;
      float s1, s2, s3, s4;
      float qDot1, qDot2, qDot3, qDot4;

      // Auxiliary variables to avoid repeated arithmetic
      float _2q1mx;
      float _2q1my;
      float _2q1mz;
      float _2q2mx;
      float _4bx;
      float _4bz;
      float _2q1 = 2.0f * q1;
      float _2q2 = 2.0f * q2;
      float _2q3 = 2.0f * q3;
      float _2q4 = 2.0f * q4;
      float _2q1q3 = 2.0f * q1 * q3;
      float _2q3q4 = 2.0f * q3 * q4;
      float q1q1 = q1 * q1;
      float q1q2 = q1 * q2;
      float q1q3 = q1 * q3;
      float q1q4 = q1 * q4;
      float q2q2 = q2 * q2;
      float q2q3 = q2 * q3;
      float q2q4 = q2 * q4;
      float q3q3 = q3 * q3;
      float q3q4 = q3 * q4;
      float q4q4 = q4 * q4;

      // Normalise accelerometer measurement
      norm = sqrt(ax * ax + ay * ay + az * az);
      if (norm == 0.0f) return; // handle NaN
      norm = 1.0f / norm;
      ax *= norm;
      ay *= norm;
      az *= norm;

      // Normalise magnetometer measurement
      norm = sqrt(mx * mx + my * my + mz * mz);
      if (norm == 0.0f) return; // handle NaN
      norm = 1.0f / norm;
      mx *= norm;
      my *= norm;
      mz *= norm;

      // Reference direction of Earth's magnetic field
      _2q1mx = 2.0f * q1 * mx;
      _2q1my = 2.0f * q1 * my;
      _2q1mz = 2.0f * q1 * mz;
      _2q2mx = 2.0f * q2 * mx;
      hx = mx * q1q1 - _2q1my * q4 + _2q1mz * q3 + mx * q2q2 + _2q2 * my * q3 + _2q2 * mz * q4 - mx * q3q3 - mx * q4q4;
      hy = _2q1mx * q4 + my * q1q1 - _2q1mz * q2 + _2q2mx * q3 - my * q2q2 + my * q3q3 + _2q3 * mz * q4 - my * q4q4;
      _2bx = sqrt(hx * hx + hy * hy);
      _2bz = -_2q1mx * q3 + _2q1my * q2 + mz * q1q1 + _2q2mx * q4 - mz * q2q2 + _2q3 * my * q4 - mz * q3q3 + mz * q4q4;
      _4bx = 2.0f * _2bx;
      _4bz = 2.0f * _2bz;

      // Gradient decent algorithm corrective step
      s1 = -_2q3 * (2.0f * q2q4 - _2q1q3 - ax) + _2q2 * (2.0f * q1q2 + _2q3q4 - ay) - _2bz * q3 * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (-_2bx * q4 + _2bz * q2) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + _2bx * q3 * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
      s2 = _2q4 * (2.0f * q2q4 - _2q1q3 - ax) + _2q1 * (2.0f * q1q2 + _2q3q4 - ay) - 4.0f * q2 * (1.0f - 2.0f * q2q2 - 2.0f * q3q3 - az) + _2bz * q4 * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (_2bx * q3 + _2bz * q1) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + (_2bx * q4 - _4bz * q2) * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
      s3 = -_2q1 * (2.0f * q2q4 - _2q1q3 - ax) + _2q4 * (2.0f * q1q2 + _2q3q4 - ay) - 4.0f * q3 * (1.0f - 2.0f * q2q2 - 2.0f * q3q3 - az) + (-_4bx * q3 - _2bz * q1) * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (_2bx * q2 + _2bz * q4) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + (_2bx * q1 - _4bz * q3) * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
      s4 = _2q2 * (2.0f * q2q4 - _2q1q3 - ax) + _2q3 * (2.0f * q1q2 + _2q3q4 - ay) + (-_4bx * q4 + _2bz * q2) * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (-_2bx * q1 + _2bz * q3) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + _2bx * q2 * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
      norm = sqrt(s1 * s1 + s2 * s2 + s3 * s3 + s4 * s4);    // normalise step magnitude
      norm = 1.0f / norm;
      s1 *= norm;
      s2 *= norm;
      s3 *= norm;
      s4 *= norm;

      // Compute rate of change of quaternion
      qDot1 = 0.5f * (-q2 * gx - q3 * gy - q4 * gz) - beta * s1;
      qDot2 = 0.5f * (q1 * gx + q3 * gz - q4 * gy) - beta * s2;
      qDot3 = 0.5f * (q1 * gy - q2 * gz + q4 * gx) - beta * s3;
      qDot4 = 0.5f * (q1 * gz + q2 * gy - q3 * gx) - beta * s4;

      // Integrate to yield quaternion
      q1 += qDot1 * deltat;
      q2 += qDot2 * deltat;
      q3 += qDot3 * deltat;
      q4 += qDot4 * deltat;
      norm = sqrt(q1 * q1 + q2 * q2 + q3 * q3 + q4 * q4);    // normalise quaternion
      norm = 1.0f / norm;
      q[0] = q1 * norm;
      q[1] = q2 * norm;
      q[2] = q3 * norm;
      q[3] = q4 * norm;

    }



    // Similar to Madgwick scheme but uses proportional and integral filtering on the error between estimated reference vectors and
    // measured ones.
    void MahonyQuaternionUpdate(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz)
    {
      float q1 = q[0], q2 = q[1], q3 = q[2], q4 = q[3];   // short name local variable for readability
      float norm;
      float hx, hy, bx, bz;
      float vx, vy, vz, wx, wy, wz;
      float ex, ey, ez;
      float pa, pb, pc;

      // Auxiliary variables to avoid repeated arithmetic
      float q1q1 = q1 * q1;
      float q1q2 = q1 * q2;
      float q1q3 = q1 * q3;
      float q1q4 = q1 * q4;
      float q2q2 = q2 * q2;
      float q2q3 = q2 * q3;
      float q2q4 = q2 * q4;
      float q3q3 = q3 * q3;
      float q3q4 = q3 * q4;
      float q4q4 = q4 * q4;

      // Normalise accelerometer measurement
      norm = sqrt(ax * ax + ay * ay + az * az);
      if (norm == 0.0f) return; // handle NaN
      norm = 1.0f / norm;        // use reciprocal for division
      ax *= norm;
      ay *= norm;
      az *= norm;

      // Normalise magnetometer measurement
      norm = sqrt(mx * mx + my * my + mz * mz);
      if (norm == 0.0f) return; // handle NaN
      norm = 1.0f / norm;        // use reciprocal for division
      mx *= norm;
      my *= norm;
      mz *= norm;

      // Reference direction of Earth's magnetic field
      hx = 2.0f * mx * (0.5f - q3q3 - q4q4) + 2.0f * my * (q2q3 - q1q4) + 2.0f * mz * (q2q4 + q1q3);
      hy = 2.0f * mx * (q2q3 + q1q4) + 2.0f * my * (0.5f - q2q2 - q4q4) + 2.0f * mz * (q3q4 - q1q2);
      bx = sqrt((hx * hx) + (hy * hy));
      bz = 2.0f * mx * (q2q4 - q1q3) + 2.0f * my * (q3q4 + q1q2) + 2.0f * mz * (0.5f - q2q2 - q3q3);

      // Estimated direction of gravity and magnetic field
      vx = 2.0f * (q2q4 - q1q3);
      vy = 2.0f * (q1q2 + q3q4);
      vz = q1q1 - q2q2 - q3q3 + q4q4;
      wx = 2.0f * bx * (0.5f - q3q3 - q4q4) + 2.0f * bz * (q2q4 - q1q3);
      wy = 2.0f * bx * (q2q3 - q1q4) + 2.0f * bz * (q1q2 + q3q4);
      wz = 2.0f * bx * (q1q3 + q2q4) + 2.0f * bz * (0.5f - q2q2 - q3q3);

      // Error is cross product between estimated direction and measured direction of gravity
      ex = (ay * vz - az * vy) + (my * wz - mz * wy);
      ey = (az * vx - ax * vz) + (mz * wx - mx * wz);
      ez = (ax * vy - ay * vx) + (mx * wy - my * wx);
      if (Ki > 0.0f)
      {
        eInt[0] += ex;      // accumulate integral error
        eInt[1] += ey;
        eInt[2] += ez;
      }
      else
      {
        eInt[0] = 0.0f;     // prevent integral wind up
        eInt[1] = 0.0f;
        eInt[2] = 0.0f;
      }

      // Apply feedback terms
      gx = gx + Kp * ex + Ki * eInt[0];
      gy = gy + Kp * ey + Ki * eInt[1];
      gz = gz + Kp * ez + Ki * eInt[2];

      // Integrate rate of change of quaternion
      pa = q2;
      pb = q3;
      pc = q4;
      q1 = q1 + (-q2 * gx - q3 * gy - q4 * gz) * (0.5f * deltat);
      q2 = pa + (q1 * gx + pb * gz - pc * gy) * (0.5f * deltat);
      q3 = pb + (q1 * gy - pa * gz + pc * gx) * (0.5f * deltat);
      q4 = pc + (q1 * gz + pa * gy - pb * gx) * (0.5f * deltat);

      // Normalise quaternion
      norm = sqrt(q1 * q1 + q2 * q2 + q3 * q3 + q4 * q4);
      norm = 1.0f / norm;
      q[0] = q1 * norm;
      q[1] = q2 * norm;
      q[2] = q3 * norm;
      q[3] = q4 * norm;

    }

};

#endif
