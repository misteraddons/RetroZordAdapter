/*******************************************************************************
 * RetroZordAdapter Version 1.00
 * by Matheus Fraguas (sonik-br)
 * 
 * https://github.com/sonik-br/RetroZordAdapter
*/

#include "Shared.h"
#include "Saturn.h"
//#include "Psx.h"
#include "Snes.h"

DeviceEnum deviceMode = RZORD_NONE;

// 315deg = 0.5V = 186
// 0deg   = 1.2V = 254
// 45deg  = 2.1V = 430
// 90deg  = 3.1V = 635
// 135deg = 4.0V = 819
// 180deg = 4.5V = 922
// 225deg = 4.7V = 956
// 270deg = 5.0V = 1024

void setup() {
  Serial.begin(9600);
  int sensorValue = analogRead(A3);
  Serial.println(sensorValue);
  if (sensorValue < 200)//switch position left
    deviceMode = RZORD_SATURN;
  else if (sensorValue < 300)//switch position right
    deviceMode = RZORD_SNES;
  else//switch position middle
    deviceMode = RZORD_PSX;

  Serial.println(deviceMode);
  pinMode(LED_BUILTIN, OUTPUT);

  switch(deviceMode){
    case RZORD_SATURN:
      saturnSetup();
      break;
    case RZORD_SNES:
      snesSetup();
      break;
    //case RZORD_PSX:
    //  psxSetup();
    //  break;
    default:
      break;
  }
}

void loop() {
 switch(deviceMode){
  case RZORD_SATURN:
    saturnLoop();
    break;
  case RZORD_SNES:
    snesLoop();
    break;
  //case RZORD_PSX:
  //  psxLoop();
  //  break;
  default:
    break;
 }
}
