#include <SoftwareSerial.h>

SoftwareSerial mySerial(22, 21); // RX, TX

// unsigned int pm1 = 0;
unsigned int pm2_5 = 0;
// unsigned int pm10 = 0;

void dustSetup() {
  Serial.begin(115200);
  while (!Serial);
  mySerial.begin(9600);
  delay(3000);
}

float dustRead() {
  int index = 0;
  char value;
  char previousValue = 0;
  
  // Serial.println("Reading from sensor...");

  while (mySerial.available()) {
    value = mySerial.read();
    if ((index == 0 && value != 0x42) || (index == 1 && value != 0x4D)){
      Serial.println("Cannot find the data header.");
      break;
    }

    if (index == 4 || index == 6 || index == 8 || index == 10 || index == 12 || index == 14) {
      previousValue = value;
    }
    // else if (index == 5) {
    //   pm1 = 256 * previousValue + value;
    //   Serial.print("{ ");
    //   Serial.print("\"pm1\": ");
    //   Serial.print(pm1);
    //   Serial.print(" ug/m3, ");
    // }
    else if (index == 7) {
      pm2_5 = 256 * previousValue + value;
      // Serial.print("\"pm2_5\": ");
      // Serial.print(pm2_5);
      // Serial.print(" ug/m3, ");
    }
    // else if (index == 9) {
    //   pm10 = 256 * previousValue + value;
    //   Serial.print("\"pm10\": ");
    //   Serial.print(pm10);
    //   Serial.print(" ug/m3");
    // }
  
    if (index > 15) {
      break;
    }
    index++;
  }
  while(mySerial.available()) mySerial.read();
  // Serial.println(" }");
  delay(1000);
  return pm2_5;
}
