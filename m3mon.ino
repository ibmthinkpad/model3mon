/* Tesla Model 3 battery monitor
 * By Roderik Leijssen
 *
 * V2 for FlexCAN and Teensy 3.2
 * https://github.com/teachop/FlexCAN_Library 
 *
 * Note: CAN messages could change with Tesla OTA updates. Please refer to the community .dbc file
 * https://github.com/joshwardell/model3dbc
*/

#include <Arduino.h>
#include <FlexCAN.h> // 
#include <SPI.h>
#include "bmsvalues.h"
#include "nexdisplay.h"

union cbuffer {
  byte bytes[8];
  uint64_t integer;
};

void printBMSData();  // Prototype for our function that dumps information to USB serial
CAN_message_t msg;
CAN_message_t inMsg;
bool candebug = 0;  // When set to 1, print raw CAN data on USB and BMS values.
long unsigned int rxId;
unsigned char len = 0;
char msgString[128]; // Used exclusively for CAN debug function
uint64_t message; // This allocated space holds the integer value of the CAN message. Used for bitwise operations.
cbuffer frame;	// We will buffer the CAN messages in here to access the 64bit value. Neccesary for bitwise operations.
BMSValues bms;	// Our class for storing sensor information
Nexdisplay display(bms, Serial2); // Optional Nextion object that uses our sensor values and Arduino Serial.

void setup() {
  Serial.begin(115200); // USB serial for dumping debug information
  Serial2.begin (115200); // Start Serial2 comunication at baud=9600, to be used with Nextion HMI
  Serial.println("Starting up!");

  Can0.begin(500000); // Start our CAN interface on Teensy at 500Kbs
}

void loop() {
  while (Can0.available()) {
    canread();
  }
}	

void canread() {
  Can0.read(inMsg); //inMesg returns the data in type CAN_message_t which is an array of bytes.
  memcpy(frame.bytes, &inMsg.buf, inMsg.len); // Copy RX data in our union for conversion.
  
  switch (inMsg.id) {
    case 0x332: // ID332BattCellMinMax
      if ((frame.integer & 0x03) == 0) {		                 // MUX : start|len (scale,offset) [min|max]
        message = ((frame.integer & 0x3C) >> 2);                 // m0 : 2|4@1+ (1,0) [0|0]
        bms.BattCellTempMaxNum = (uint8_t)(message);

        message = ((frame.integer & 0xF00) >> 8);                // m0 : 8|4@1+ (1,0) [0|0]
        bms.BattCellTempMinNum = (uint8_t)(message);

        message = ((frame.integer & 0xFF0000) >> 16);            // m0 : 16|8@1+ (0.5,-40) [0|0] "C"
        bms.BattCellTempMax = (float)((0.5 * message) - 40);

        message = ((frame.integer & 0xFF000000) >> 24);          // m0 : 24|8@1+ (0.5,-40) [0|0] "C"
        bms.BattCellTempMin = (float)((0.5 * message) - 40);

        message = ((frame.integer & 0xFF00000000) >> 32);        // m0 : 32|8@1+ (0.5,-40) [0|0] "C"
        bms.BattCellModelTMax = (float)((0.5 * message) - 40);

        message = ((frame.integer & 0x7F0000000000) >> 40);      // m0 : 40|8@1+ (0.5,-40) [0|0] "C"
        bms.BattCellModelTMin = (float)((0.5 * message) - 40);
      } else {								                     // mux is now 1
        message = ((frame.integer & 0x3FFC) >> 2);               // m1 : 2|12@1+ (0.002,0) [0|0] "V"
        bms.BattCellBrickVoltageMax = (float)(0.002 * message);

        message = ((frame.integer & 0xFFF0000) >> 16);           // m1 : 16|12@1+ (0.002,0) [0|0] "V"
        bms.BattCellBrickVoltageMin = (float)(0.002 * message);

        message = ((frame.integer & 0x7F00000000) >> 32);        // m1 : 32|7@1+ (1,1) [0|0]
        bms.BattCellBrickVoltageMaxNum = (uint16_t)(message + 1);

        message = ((frame.integer & 0x7F0000000000) >> 40);      // m1 : 40|7@1+ (1,1) [0|0]
        bms.BattCellBrickVoltageMinNum = (uint16_t)(message + 1);
      }
    break;

    case 0x3B2: // ID3B2BMS_log2
      if ((frame.integer & 0x1F) == 0) {                       // Mux is now 0, size is 5 bits
        
        message = ((frame.integer & 0x3FFE0) >> 6);            // BMS_cacAvg m0 : 6|13@1+ (0.1,0) [0|819.1] "Ah"  Receiver
        bms.bmscacavg = (float)(0.1 * message);
        
        message = ((frame.integer & 0xFFF80000) >> 19);        // BMS_cacMin m0 : 19|13@1+ (0.1,0) [0|819.1] "Ah"  Receiver
        bms.bmscacmin = (float)(0.1 * message);
        
        message = ((frame.integer & 0x1FFF0000000000) >> 40);  // BMS_cacMax m0 : 40|13@1+ (0.1,0) [0|819.1] "Ah"  Receiver
        bms.bmscacmax = (float)(0.1 * message);
      } else if ((frame.integer & 0x1F) == 6) {                // Mux is now 6
        message = ((frame.integer & 0xFFFFFF00) >> 8);         // BMS_ahChargeTotal m6 : 8|24@1+ (0.1,0) [0|1677721.5] "Ah"  Receiver
        bms.totalchargedah = (float)(0.1 * message);
        
        message = ((frame.integer & 0xFFFFFF00000000) >> 32);  // BMS_ahDischargeTotal m6 : 32|24@1+ (0.1,0) [0|1677721.5] "Ah"  Receiver
        bms.totaldischargedah = (float)(0.1 * message);
      }
	  // fixme: screen updates are slow unless I update the screen at the slowest CAN message on the bus
	  display.update(); // Sends stored data to our Nextion HMI via its initialised Serial reference and BMSValues
    break;

    case 0x132: // ID132HVBattAmpVolt
      message = (frame.integer & 0xFFFF);
      bms.battvoltage = (float)(0.01 * message);                  // BattVoltage132 : 0|16@1+ (0.01,0) [0|655.35] "V"  Receiver

      message = ((frame.integer & 0xFFFF0000) >> 16);             // SmoothBattCurrent132 : 16|16@1- (-0.1,0) [-3276.7|3276.7] "A"  Receiver
      if ((float)message > -3276.7 && ((float)message < 3276.7))
        bms.smoothcurrenta = (float)(-0.1 * message);

      message = ((frame.integer & 0xFFFF00000000) >> 32);        // RawBattCurrent132 : 32|16@1- (-0.05,500) [-1138.35|2138.4] "A"  Receiver
      if ((float)message > -1138.35 && (float)message < 2138.4)
        bms.rawcurrenta = (float)((-0.05 * message) + 500);
    break;

    case 0x2D2:	// BMS configured Min Voltage V
      message = (frame.integer & 0xFFFF);                  // MinVoltage2D2 : 0|16@1+ (0.01,0) [0|430] "V"  Receiver
      bms.bmsminv = (float)(0.01 * message);

      message = ((frame.integer & 0xFFFF0000) >> 16);      // MaxVoltage2D2 : 16|16@1+ (0.01,0) [0|430] "V"  Receiver
      bms.bmsmaxv = (float)(0.01 * message);
    break;

    case 0x3D2: // TotalChargeDischarge
      message = (frame.integer & 0xFFFFFFFF);                  // TotalDischargeKWh3D2 : 0|32@1+ (0.001,0) [0|4294967.295] "kWh"  Receiver
      if ((float)message < 4294967.295)
        bms.totaldischargedkwh = (float)(0.001 * message);
        message = (frame.integer & 0xFFFFFFFF00000000) >> 32;  // TotalChargeKWh3D2 : 32|32@1+ (0.001,0) [0|4294967.295] "kWh"  Receiver
      if ((float)message < 4294967.295)
        bms.totalchargedkwh = (float)(0.001 * message);
      break;

    case 0x292: // ID292BMS_SOC
      message = (frame.integer & 0x3FF);                     // SOCUI292 : 10|10@1+ (0.1,0) [0|102.3] "%"  Receiver
      bms.ui_soc = (float)(0.1 * message);

      message = ((frame.integer & 0xFFC0000000) >> 30);      // SOCave292 : 30|10@1+ (0.1,0) [0|102.3] "%"  Receiver
      bms.avg_soc = (float)(0.1 * message);
    break;

    case 0x3F2: // BMS counters
	  if ((frame.integer & 0x7) == 0) {						   // BMSCountersIndex3F2 M : 0|3@1+ (1,0) [0|0] ""  Receiver
         message = ((frame.integer & 0xFFFFFFFF00) >> 8);      // BMStotalACcharge3F2 m0 : 8|32@1+ (0.001,0) [0|4294967.295] "KWh"  Receiver
         bms.bmstotalaccharge = (float)(0.001 * message);
       } else if ((frame.integer & 0x7) == 1) {                // Mux is now 1
         message = ((frame.integer & 0xFFFFFFFF00) >> 8);      // BMStotalDCcharge3F2 m1 : 8|32@1+ (0.001,0) [0|4294967.295] "KWh"  Receiver
         bms.bmstotaldccharge = (float)(0.001 * message);
	   }
    break;
    default:
      //Serial.println("Got message but no filtered messages to display!");
    break;
  }

  if (candebug == 1) {
    Serial.print(millis());
    if ((inMsg.id & 0x80000000) == 0x80000000)    // Determine if ID is standard (11 bits) or extended (29 bits)
      sprintf(msgString, "Extended ID: 0x%.8lX  DLC: %1d  Data:", (inMsg.id & 0x1FFFFFFF), inMsg.len);
    else
      sprintf(msgString, ",0x%.3lX,false,%1d", inMsg.id, inMsg.len);

    Serial.print(msgString);

    if ((inMsg.id & 0x40000000) == 0x40000000) {  // Determine if message is a remote request frame.
      sprintf(msgString, " REMOTE REQUEST FRAME");
      Serial.print(msgString);
    } else {
      for (byte i = 0; i < inMsg.len; i++) {
        sprintf(msgString, ", 0x%.2X", inMsg.buf[i]);
        Serial.print(msgString);
      }
    }
    Serial.println();
	//printBMSData()
  }
}

void printBMSData() {
  Serial.print("BattCellTempMaxNum:\t\t"); Serial.println(bms.BattCellTempMaxNum);
  Serial.print("BattCellTempMinNum:\t\t"); Serial.println(bms.BattCellTempMinNum);
  Serial.print("BattCellTempMax:\t\t"); Serial.println(bms.BattCellTempMax);
  Serial.print("BattCellTempMin:\t\t"); Serial.println(bms.BattCellTempMin);
  Serial.print("BattCellModelTMax:\t\t"); Serial.println(bms.BattCellModelTMax);
  Serial.print("BattCellModelTMin:\t\t"); Serial.println(bms.BattCellModelTMin);
  Serial.print("BattCellBrickVoltageMax:\t"); Serial.println(bms.BattCellBrickVoltageMax);
  Serial.print("BattCellBrickVoltageMin:\t"); Serial.println(bms.BattCellBrickVoltageMin);
  Serial.print("BattCellBrickVoltageMaxNum:\t"); Serial.println(bms.BattCellBrickVoltageMaxNum);
  Serial.print("BattCellBrickVoltageMinNum:\t"); Serial.println(bms.BattCellBrickVoltageMinNum);
  Serial.print("Interface SoC:\t\t\t"); Serial.println(bms.ui_soc);
  Serial.print("Average SoC:\t\t\t"); Serial.println(bms.avg_soc);
  Serial.print("Battery voltage:\t\t"); Serial.println(bms.battvoltage);
  Serial.print("Battery raw current:\t\t"); Serial.println(bms.rawcurrenta);
  Serial.print("Battery smooth current:\t\t"); Serial.println(bms.smoothcurrenta);
  Serial.print("BMS min V:\t\t\t"); Serial.println(bms.bmsminv);
  Serial.print("BMS max V:\t\t\t"); Serial.println(bms.bmsmaxv);
  Serial.print("BMS AC charge:\t\t\t"); Serial.println(bms.bmstotalaccharge);
  Serial.print("BMS DC charge:\t\t\t"); Serial.println(bms.bmstotaldccharge);
  Serial.print("BMS total discharged kWh:\t"); Serial.println(bms.totaldischargedkwh);
  Serial.print("BMS total charged kWh:\t\t"); Serial.println(bms.totalchargedkwh);
  Serial.print("BMS total discharged Ah:\t"); Serial.println(bms.totaldischargedah);
  Serial.print("BMS total charged Ah:\t\t"); Serial.println(bms.totalchargedah);
  Serial.print("BMS Average CAC Ah:\t\t"); Serial.println(bms.bmscacavg);
  Serial.print("BMS Min CAC Ah:\t\t\t"); Serial.println(bms.bmscacmin);
  Serial.print("BMS Max CAC Ah:\t\t\t"); Serial.println(bms.bmscacmax);
  Serial.println();
}

void printBytes(unsigned char *msg, size_t len) {
  char msgBuf[64];
  for (unsigned char i = 0; i < len; i++) {
    sprintf(msgBuf, " %#x", msg[i]);
    Serial.print(msgBuf);
  }
  Serial.println();
}
