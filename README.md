# model3mon
Teensy sketch to dump Tesla Model 3 CAN bus data using FlexCAN

This sketch reads data from the Vehicle CAN bus from a Tesla Model 3 to display various values reported by the Battery HV controller. With this you can see statistics about the battery in a glance. It can also send the data to a serially attached Nextion display in a very basic way. I included the .tft and .hmi project files but they arent strictly neccesary and it can also dump the information via USB serial.
