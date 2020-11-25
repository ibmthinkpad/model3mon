#ifndef NEXDISPLAY_H
#define NEXDISPLAY_H

#include "bmsvalues.h"

class Nexdisplay {
public:
	Nexdisplay();
	Nexdisplay(BMSValues&, HardwareSerial&);
	void update();
	void sendCommand(char*, float);
	void sendCommand(char*, int);
private:
	BMSValues& values;
	HardwareSerial& port;
};

#endif // NEXDISPLAY_H
