/*

  MPU6050/AccelGyroTest
  First test of an Invensense MPU-6050 3-Axes Accelerometer Gyroscope Module

  This sketch is based on the original from http://playground.arduino.cc/Main/MPU-6050

  For info and circuit diagrams see https://github.com/tardate/LittleArduinoProjects/tree/master/playground/MPU6050/AccelGyroTest

 */

#include <SWire.h>

#define DELAY_STEP 500

#define MPU6050_ADDRESS            (0x68)

#define MPU6050_ACCEL_XOUT_H       (0x3B)


void setup() {
  Serial.begin(9600);
  SWire.begin(A3,A4);
  wakeUp();
}

void loop() {
  readAndLog();
  delay(DELAY_STEP);
}

void wakeUp() {
  SWire.beginTransmission(MPU6050_ADDRESS);
  SWire.write(0x6B);  // PWR_MGMT_1 register
  SWire.write(0);     // set to zero (wakes up the MPU-6050)
  SWire.endTransmission(true);
}

void readAndLog() {
  int16_t AcX,AcY,AcZ,GyX,GyY,GyZ;
  int16_t Tmp;

  SWire.beginTransmission(MPU6050_ADDRESS);
  SWire.write(MPU6050_ACCEL_XOUT_H);  // starting with register 0x3B (ACCEL_XOUT_H)
  SWire.endTransmission(false);
  SWire.requestFrom(0x68, 14);  // request a total of 14 registers

  AcX=SWire.read()<<8|SWire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)
  AcY=SWire.read()<<8|SWire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  AcZ=SWire.read()<<8|SWire.read();  // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
  Tmp=SWire.read()<<8|SWire.read();  // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
  GyX=SWire.read()<<8|SWire.read();  // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
  GyY=SWire.read()<<8|SWire.read();  // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
  GyZ=SWire.read()<<8|SWire.read();  // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)

  Serial.print("Accelerometer x,y,z: ");
  Serial.print(AcX, DEC); Serial.print(", ");
  Serial.print(AcY, DEC); Serial.print(", ");
  Serial.println(AcZ, DEC);

  Serial.print("Gyroscope x,y,z: ");
  Serial.print(GyX, DEC); Serial.print(", ");
  Serial.print(GyY, DEC); Serial.print(", ");
  Serial.println(GyZ, DEC);

  Serial.print("Temperature: ");
  Serial.println(Tmp/340.00+36.53); //equation for temperature in degrees C from datasheet
  Serial.println();
}

