
#include <MPU6050_tockn.h>
#include <Wire.h>

MPU6050 mpu6050(Wire);

long timer = 0;
float iz = 0;
float iy = 0;
int ythrottle = 15;
int zthrottle = 10;
float cury = 0;
float curz = 0;

void setup() {
  Serial.begin(9600);
  //Wire.begin();
  mpu6050.begin();
  mpu6050.calcGyroOffsets(true);
  int i = 0;
  for (i = 0; i < 10; i++)
  {
    mpu6050.update();
    iy = iy + mpu6050.getAngleY();
    iz = iz + mpu6050.getAngleZ();
  }
  iy = iy / 10;
  iz = iz / 10;
  Serial.print(iy);
  Serial.println(iz);
}

void loop() {
  mpu6050.update();

  if (millis() - timer > 1000) {
    cury = mpu6050.getAngleY();
    curz = mpu6050.getAngleZ();

    if ((cury - iy) < (-1 * ythrottle))
    {
      Serial.print("Up | "); //-ve value
    } else if ((cury - iy) > ythrottle)
    {
      Serial.print("Down | "); //+ve value
    } else
    {
      Serial.print("Center Y | ");
    }

    if ((curz - iz) < (-1 * zthrottle))
    {
      Serial.println("Left | "); //-ve value
    } else if ((curz - iz) > zthrottle)
    {
      Serial.println("Right | "); //+ve value
    } else
    {
      Serial.println("Center Z | ");
    }
    timer = millis();
  }
}
