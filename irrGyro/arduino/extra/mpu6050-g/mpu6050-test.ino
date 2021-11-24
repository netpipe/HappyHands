#include <MPU6050_tockn-swire.h>


MPU6050 mpu6050;

float accX_cal = 0, accY_cal = 0, accZ_cal = 0;
float accX, accY, accZ, acc_tot;
float err=0;

void setup() {
  Serial.begin(9600);

  //SWire.begin(A3,A4);
  mpu6050.begin(A3,A4);

  for (int i = 0; i < 1000; i++) {
    mpu6050.update();
    accX_cal += mpu6050.getAccX();
    accY_cal += mpu6050.getAccY();
    accZ_cal += mpu6050.getAccZ();
    delay(1);
  }
  accX_cal /= 1000;
  accY_cal /= 1000;
  accZ_cal = accZ_cal / 1000 - 1;
}

int count = 0;

void loop() {
  if (count < 1000) {

    //these lines are only to move your mpu6050 in an other position without affecting the measurement of g 
    if (count % 300 == 0){
      Serial.println("move");
      delay(2000);
    }
    mpu6050.update();
    accX = mpu6050.getAccX() - accX_cal;
    accY = mpu6050.getAccY() - accY_cal;
    accZ = mpu6050.getAccZ() - accZ_cal;
    acc_tot = sqrt(accX * accX + accY * accY + accZ * accZ);
    err += abs(acc_tot - 1);    
    count++;
    Serial.println(acc_tot);
    delay(20);
  }
  if (count == 1000){
    Serial.print("average error: ");
    Serial.println(err / count);
    count++;
  }
}
