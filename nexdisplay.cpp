#include "nexdisplay.h"

Nexdisplay::Nexdisplay(BMSValues& v, HardwareSerial& p): values(v),port(p) {} // Initialisation list for our references

void Nexdisplay::update() {
	sendCommand("cellmaxtempnum",values.BattCellTempMaxNum);
	sendCommand("cellmintempnum",values.BattCellTempMinNum);
	sendCommand("cellmaxtemp",values.BattCellTempMax);
	sendCommand("cellmintemp",values.BattCellTempMin);
	sendCommand("cellmaxvnum",values.BattCellBrickVoltageMaxNum);
	sendCommand("cellminvnum",values.BattCellBrickVoltageMinNum);
	sendCommand("cellmaxv",values.BattCellBrickVoltageMax);
	sendCommand("cellminv",values.BattCellBrickVoltageMin);
	
	sendCommand("uisoc",values.ui_soc);
	sendCommand("avgsoc",values.avg_soc);
	sendCommand("batterybar",((uint16_t)values.ui_soc)); //cast state of charge to int for battery bar
	
	sendCommand("voltage",values.battvoltage);
	sendCommand("current",values.smoothcurrenta);
	
	sendCommand("chargedkwh",values.totalchargedkwh);
	sendCommand("dischargedkwh",values.totaldischargedkwh);
	sendCommand("chargedah",values.totalchargedah);
	sendCommand("dischargedah",values.totaldischargedah);
	sendCommand("chargedac",values.bmstotalaccharge);
	sendCommand("chargeddc",values.bmstotaldccharge);
	sendCommand("cacmin",values.bmscacmin);
	sendCommand("cacavg",values.bmscacavg);
	sendCommand("cacmax",values.bmscacmax);
}

void Nexdisplay::sendCommand(char* objectid, float value) {
	char cmdbuffer[64];
	sprintf(cmdbuffer, "%s.txt=\"%4.2f\"",objectid, value); // Nextion does not support float. Therefore we send it a formatted string with two decimal places
	port.print(cmdbuffer); // write our command to serial
	port.write(0xFF); // add trailing data to indicate end of transmission
	port.write(0xFF);
	port.write(0xFF);
}

void Nexdisplay::sendCommand(char* objectid, int value) {
	char cmdbuffer[64];
	sprintf(cmdbuffer, "%s.val=%i",objectid, value);
	port.print(cmdbuffer);
	port.write(0xFF);
	port.write(0xFF);
	port.write(0xFF);
}

// You can add more custom commands by overloading this function