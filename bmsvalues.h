#ifndef BMSVALUES_H
#define BMSVALUES_H

#include <Arduino.h>

class BMSValues {
public:
  uint8_t BattCellTempMaxNum;
  uint8_t BattCellTempMinNum;
  uint8_t BattCellBrickVoltageMaxNum;
  uint8_t BattCellBrickVoltageMinNum;
  
  float BattCellTempMax;
  float BattCellTempMin;
  float BattCellModelTMax;
  float BattCellModelTMin;
  float battvoltage;
  float rawcurrenta;
  float smoothcurrenta;
  float ui_soc;	
  float avg_soc;
  float bmsminv;
  float bmsmaxv;
  float bmstotalaccharge;
  float bmstotaldccharge;
  float totalchargedkwh;
  float totaldischargedkwh;
  float bmscacavg;
  float bmscacmin;
  float bmscacmax;
  float totalchargedah;
  float totaldischargedah;
  float BattCellBrickVoltageMax;
  float BattCellBrickVoltageMin;
};

#endif // BMSVALUES_H
