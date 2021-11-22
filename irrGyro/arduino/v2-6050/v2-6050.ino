
#include <MPU6050_tockn-swire.h>
//#include <Wire.h>

#define test

MPU6050 mpu6050;
//#define test //adds in the z axis
long timer = 0;
float ix = 0;
float iz = 0;
float iy = 0;
int xthrottle = 15;
int ythrottle = 15;
int zthrottle = 10;
float curx = 0;
float cury = 0;
float curz = 0;
int count = 0;

void setup() {
  Serial.begin(9600);
  //Wire.begin();
  mpu6050.begin(A4,A5);
  calibrate();
  //   Serial.print(ix);
  // Serial.print(iy);
  // Serial.println(iz);
}

void calibrate() {
  Serial.println("calibrating");

  mpu6050.calcGyroOffsets(true);
  int i = 0;
  for (i = 0; i < 10; i++)
  {
    mpu6050.update();
    ix = ix + mpu6050.getAngleX();
    iy = iy + mpu6050.getAngleY();
    iz = iz + mpu6050.getAngleZ();
  }
  ix = ix / 10;
  iy = iy / 10;
  iz = iz / 10;
}



void loop() {

  if (count > 3000) {
    //calibrate();
    iz = iz - 1;
    count = 0;
  }
  mpu6050.update();

  if (millis() - timer > 40) {
    curx = mpu6050.getAngleX();
    cury = mpu6050.getAngleY();
#ifdef test
    curz = mpu6050.getAngleZ();
#endif
    // Serial.print(" | X");
    Serial.print(curx);
    Serial.print(":");
    //Serial.print(" | Y");
#ifdef test
    Serial.print(cury);
    Serial.print(":");
    Serial.println(curz);
#else
    Serial.println(cury);
    Serial.print(" | Z");
    Serial.println(curz);
#endif

    //  Serial.println(mpu6050.getAccY()); //-ve value
#ifdef comlink
    if ((curx - ix) < (-1 * xthrottle))
#ifdef test
    {
      Serial.print("XDown | "); //-ve value
    } else if ((curx - ix) > xthrottle)
    {
      Serial.print("XUp | "); //+ve value
    } else
#else
    {
      Serial.print("XUp | "); //-ve value
    } else if ((curx - ix) > xthrottle)
    {
      Serial.print("XDown | "); //+ve value
    } else
#endif
    {
      Serial.print("Center X | ");
    }

    if ((cury - iy) < (-1 * ythrottle))

#ifdef test
    {
      Serial.print("Up | "); //-ve value
    } else if ((cury - iy) > ythrottle)
    {
      Serial.print("Down | "); //+ve value
    } else
#else
    {
      Serial.print("YLeft | "); //-ve value
    } else if ((cury - iy) > ythrottle)
    {
      Serial.print("YRight | "); //+ve value
    } else
#endif
    {
      Serial.print("Center Y | ");
    }

#ifdef test
    if ((curz - iz) < (-1 * zthrottle))
    {
      Serial.print("Right | "); //-ve value
    } else if ((curz - iz) > zthrottle)
    {
      Serial.print("Left | "); //+ve value
    } else
    {
      Serial.print("Center Z | ");
    }
#endif

#endif //comlink

    timer = millis();
    count++;
    delay(10);
  }

  //timer = millis();
  //

}
