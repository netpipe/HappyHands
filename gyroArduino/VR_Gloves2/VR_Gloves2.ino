//#include <HardwareSerial.h>
//#include <HID.h>
#include "GY521.h"

long timer = 0;

GY521 Main(0x68);//, Thumb(0x68), Point(0x68), Middle(0x68), Ring(0x68), Little(0x68);
uint32_t counter = 0;
const int avg = 15;
float IX = 0;
float IY = 0;
float IZ = 0;
float ACX = 0;
float ACY = 0;
float ACZ = 0;
float aax = 0;
float aay = 0;
float aaz = 0;
float aax2 = 0;
float aay2 = 0;
float aaz2 = 0;
int SensitivtyFactor = 100;
void CallibrateSensors()
{
  Main.axe = 0; // Thumb.axe = Point.axe = Middle.axe = Ring.axe = Little.axe = 0;
  Main.aye = 0; // Thumb.aye = Point.aye = Middle.aye = Ring.aye = Little.aye = 0;
  Main.aze = 0; // Thumb.aze = Point.aze = Middle.aze = Ring.aze = Little.aze = 0;
  Main.gxe = 0; // Thumb.gxe = Point.gxe = Middle.gxe = Ring.gxe = Little.gxe = 0;
  Main.gye = 0; // Thumb.gye = Point.gye = Middle.gye = Ring.gye = Little.gye = 0;
  Main.gze = 0; // Thumb.gze = Point.gze = Middle.gze = Ring.gze = Little.gze = 0;
}


void setup() {

  Serial.begin(9600);
  delay(5000);
  while (!Serial)
  {
    delay(100);
  }
  Serial.println("Started");
  //Starting sensors as I2C port
//  if (      
Wire.begin();
    //Main.begin(A4, A5)) {
  //  Serial.println("Init Ok");
  //}
  //else {
  //  Serial.println("Init Failed");
  //}//(SDA,SCL)
  /*
    Thumb.begin(5, 4); //(SDA,SCL)
    Point.begin(7, 6); //(SDA,SCL)
    Middle.begin(9, 8); //(SDA,SCL)
    Ring.begin(11, 10); //(SDA,SCL)
    Little.begin(13, 12); //(SDA,SCL)
  */
  //Setting Senstivity

    Main.setAccelSensitivity(2);  // 2g
  Main.setGyroSensitivity(0);   // 250 degrees/s
  
 // Serial.println(Main.setAccelSensitivity(2));  // 8g
 // Main.setGyroSensitivity(1);   // 500 degrees/s
  Main.setThrottle();
  /*
    //Thumb Sensor
    Thumb.setAccelSensitivity(2);  // 8g
    Thumb.setGyroSensitivity(1);   // 500 degrees/s
    Thumb.setThrottle();
    //Point Sensor
    Point.setAccelSensitivity(2);  // 8g
    Point.setGyroSensitivity(1);   // 500 degrees/s
    Point.setThrottle();
    //Middle Sensor
    Middle.setAccelSensitivity(2);  // 8g
    Middle.setGyroSensitivity(1);   // 500 degrees/s
    Middle.setThrottle();
    //Ring Sensor
    Ring.setAccelSensitivity(2);  // 8g
    Ring.setGyroSensitivity(1);   // 500 degrees/s
    Ring.setThrottle();
    //Little Sensor
    Little.setAccelSensitivity(2);  // 8g
    Little.setGyroSensitivity(1);   // 500 degrees/s
    Little.setThrottle();
  */
  //Callibrate all values
  CallibrateSensors();
  for (int i = 0; i < avg; i++)
  {
    Main.read();
        IX = IX + Main.getAngleX();
    IY = IY + Main.getAngleY();
    IZ = IZ + Main.getAngleZ();
    
   // IX = IX + Main.getPitch();
   // IY = IY + Main.getRoll();
   // IZ = IZ + Main.getYaw();
    ACX = ACX + Main.getAccelX();
    ACY = ACY + Main.getAccelY();
    ACZ = ACZ + Main.getAccelZ();
  }
  IX = IX / avg;
  IY = IY / avg;
  IZ = IZ / avg;
    ACX = ACX / avg;
    ACY = ACY / avg;
    ACZ = ACZ / avg;
  Serial.print("ix=");
  Serial.println(IX);
  Serial.print("iy=");
  Serial.println(IY);
  Serial.print("iz=");
  Serial.println(IZ);

  // try moving hand certain ammount or distance to calibrate ?
  
  delay(100);
}


/*
  int PIY = Point.getAngleY();
  int PIZ = Point.getAngleZ();

  int MIY = Middle.getAngleY();
  int MIZ = Middle.getAngleZ();
*/

  float ax = 0;
  float ay = 0;
  float az = 0;


//static position reading for screen
  float posx = 0;
  float posy = 0;
  float posz = 0;



uint32_t then;


  void test(){
   // Serial.println("test");
    int factor=1;
     aax = Main.getAccelX()*factor;
    aay =  Main.getAccelY()*factor;
    aaz =  Main.getAccelZ()*factor;
//#define aprint
#ifdef aprint
  Serial.print("ax=");  Serial.print(ax);
  Serial.print("ay=");  Serial.print(ay);
  Serial.print("az=");  Serial.println(az);
      #endif

      ax=aax;
      ay=aay;
      az=aaz;

  //posx += ax;
  //posy += ay;
  //posz += az;
  
      posx += ax-aax2;
      posy += ay-aay2;
      posz += az-aaz2;
       aax2=aax;
       aay2=aay;
       aaz2=aaz;
    }



    
  bool btest=true;
  float arx = 0;
  float ary = 0;
  float arz = 0;

  
void loop()
{
  
  if (millis() - timer > 500) {
    uint32_t start = micros();
  //  then = start;
  float x = 0;
  float y = 0;
  float z = 0;
    float xa = 0;
  float ya = 0;
  float za = 0;
  ax = 0;
  ay = 0;
  az = 0;
  int avg=6;
 //delay(100);
 // delay(SensitivtyFactor);
  for (int i = 0; i < avg; i++)
  {
    Main.read();
    
    xa = xa + Main.getAngleX();
    ya = ya + Main.getAngleY();
    za = za + Main.getAngleZ();

    x = x + Main.getPitch();
    y = y + Main.getRoll();
    z = z + Main.getYaw();

    test(); //gets accelerometer data
 
  //  if (btest){ test(); btest = !btest;   }else{btest = !btest;}
    
  }


  x = x / avg;
  y = y / avg;
  z = z / avg;

  xa = xa / avg;
  ya = ya / avg;
  za = za / avg;

  // average not needed for accel
  ax = ax / (avg);
  ay = ay / (avg);
  az = az / (avg);
 //   ax = ax / (avg2/2 );
 // ay = ay / (avg2/2);
 // az = az / (avg2/2);
// velocity = distance / time   so distance = time / velocity
// use both the acceleration and the angle data to figure out the distance traveled over time
      uint32_t duration = micros() - start;
      duration=1;    
    arx = duration * (ACX - ax);
    ary = duration * (ACY - ay);
    arz = duration * (ACZ - az);   
  //  arx = (ACX - ax);
  //  ary = (ACY - ay);
  //  arz = (ACZ - az);
    //arx = map(arx, 0, 4000, -1, 1);
    //ary = map(ary, 0, 4000, -1, 1);
    //arz = map(arz, 0, 4000, -1, 1);

  //zerocalibrated values
  float rx = IX - x;
  float ry = IY - y;
  float rz = IZ - z;
  
//  #define mprint
  #ifdef mprint
  //gyro rotation axis
  Serial.print("x=");
  Serial.print(x);
  Serial.print("y=");
  Serial.print(y);
  Serial.print("z=");
  Serial.println(z);
#endif
  #define mprinta
  #ifdef mprinta
  //gyro rotation axis
  #define comlink
    #ifdef comlink
    //  Serial.print("xa=");
      Serial.print(xa);
      Serial.print(":");
      Serial.print(ya);
      Serial.print(":");
      Serial.println(za);
    #else
      Serial.print("xa=");
      Serial.print(xa);
      Serial.print("ya=");
      Serial.print(ya);
      Serial.print("za=");
      Serial.println(za);
    #endif
#endif

//  #define mrprint2

   //  #define mprintaa  //aax
    //#define mprintaa2


     
  #ifdef mrprint2  //rotation
    Serial.print("rx=");
  Serial.print(rx);
  Serial.print("ry=");
  Serial.print(ry);
  Serial.print("rz=");
  Serial.println(rz);
  #endif

      #ifdef mprintaa
  Serial.print("aax=");
  Serial.print(aax);
  Serial.print("aay=");
  Serial.print(aay);
  Serial.print("aaz=");
  Serial.println(aaz);
  #endif
  

  #ifdef mprintaa2
  Serial.print("aax3=");
  Serial.print(aax-aax2);
  Serial.print("aay3=");
  Serial.print(aaz-aaz2);
  Serial.print("aaz3=");
  Serial.println(aaz-aaz2);
  #endif
  
 // #define mprint2
  #ifdef mprint2
  Serial.print("ax=");
  Serial.print(arx);
  Serial.print("ay=");
  Serial.print(arz);
  Serial.print("az=");
  Serial.println(arz);
  #endif
  
 // #define posprint
  #ifdef posprint
  Serial.print("posx=");
  Serial.print(posx);
  Serial.print("posy=");
  Serial.print(posy);
  Serial.print("posz=");
  Serial.println(posz);
  #endif


   //   mouseX = map(accel.x, 0, 4000, -1, 1);
   // mouseY = map(accel.y, 0, 4000, -1, 1);
    
  //  dx = int(math.floor((accelX - midAccelX))*cursorSpeed/500) 
  
  //  ax -= Main.getAccelX();
  //  ay -= Main.getAccelY();
  //  az -= Main.getAccelZ();
  /*
    if ((abs(rz) >= 30) && (abs(rz) <= 40))
    {
      if (rz > 0)
        Serial.println("0, -3, 0"); //if used Leanardo or any USB OTG device replace Serial.println with mouse.move
      else if (rz < 0)
        Serial.println("0, 3, 0");
    }

    else if ((abs(rz) >= 40) && (abs(rz) <= 60)) {
      if (rz > 0)
        Serial.println("0, -6, 0");
      else if (rz < 0)
        Serial.println("0, 6, 0");
    }

    if ((abs(rx) >= 30) && (abs(rx) <= 40)) {
      if (rx > 0)
        Serial.println("3, 0, 0");
      else if (rx < 0)
        Serial.println("-3, 0, 0");
    }
    else if ((abs(rx) >= 40) && (abs(rx) <= 60)) {
      if (rx > 0)
        Serial.println("6, 0, 0");
      else if (rx < 0)
        Serial.println("-6, 0, 0");
    }

    Thumb.read();
    Point.read();
    z = Point.getAngleZ();
    rz = PIZ - z;
    if (abs(rz) > PIZ)
    {
      Serial.print("Mouse.click(MOUSE_LEFT)f55");
    }
    Middle.read();
    z = Middle.getAngleZ();
    rz = MIZ - z;
    if (abs(rz) > MIZ)
    {
      Serial.print("Mouse.click(MOUSE_RIGHT)");
    }
    Ring.read();
    Little.read();
  */
    timer = millis();
  }

}
