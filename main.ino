#include <HardwareSerial.h>
#include <HID.h>
#include "GY521.h"

GY521 Main(0x68), Thumb(0x68), Point(0x68), Middle(0x68), Ring(0x68), Little(0x68);
uint32_t counter = 0;

int SensitivtyFactor = 100;
void CallibrateSensors()
{
  Main.axe = Thumb.axe = Point.axe = Middle.axe = Ring.axe = Little.axe = 0;
  Main.aye = Thumb.aye = Point.aye = Middle.aye = Ring.aye = Little.aye = 0;
  Main.aze = Thumb.aze = Point.aze = Middle.aze = Ring.aze = Little.aze = 0;
  Main.gxe = Thumb.gxe = Point.gxe = Middle.gxe = Ring.gxe = Little.gxe = 0;
  Main.gye = Thumb.gye = Point.gye = Middle.gye = Ring.gye = Little.gye = 0;
  Main.gze = Thumb.gze = Point.gze = Middle.gze = Ring.gze = Little.gze = 0;
}


void setup() {

  Serial.begin(115200);
  Serial.println("Started");
  //Starting sensors as I2C port
  Main.begin(3, 2); //(SDA,SCL)
  Thumb.begin(5, 4); //(SDA,SCL)
  Point.begin(7, 6); //(SDA,SCL)
  Middle.begin(9, 8); //(SDA,SCL)
  Ring.begin(11, 10); //(SDA,SCL)
  Little.begin(13, 12); //(SDA,SCL)
  //Setting Senstivity
  Main.setAccelSensitivity(2);  // 8g
  Main.setGyroSensitivity(1);   // 500 degrees/s
  Main.setThrottle();
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
  //Callibrate all values
  CallibrateSensors();
  Main.read();

}
int IX = Main.getAngleX();
int IZ = Main.getAngleZ();

int PIY = Point.getAngleY();
int PIZ = Point.getAngleZ();

int MIY = Middle.getAngleY();
int MIZ = Middle.getAngleZ();

void loop()
{
  delay(SensitivtyFactor);
  Main.read();
  int x = Main.getAngleX();
  int z = Main.getAngleZ();
  int rz = IZ - z;
  int rx = IX - x;

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
  int z = Point.getAngleZ();
  int rz = PIZ - z;
  if (abs(rz)>PIZ)
  {
    Serial.print("Mouse.click(MOUSE_LEFT)")
  }
  Middle.read();
  int z = Middle.getAngleZ();
  int rz = MIZ - z;
  if ((abs(rz)>MIZ)
  {
    Serial.print("Mouse.click(MOUSE_RIGHT)")
  }
  Ring.read();
  Little.read();

}
