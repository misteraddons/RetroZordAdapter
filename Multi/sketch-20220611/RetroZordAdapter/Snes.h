/*******************************************************************************
 * Snes controller input library.
 * https://github.com/sonik-br/SnesLib
*/

//Uncomment to enable multitap support. Requires wiring two additional pins.
//#define SNES_ENABLE_MULTITAP

#include "src/SnesLib/SnesLib.h"
#include "src/ArduinoJoystickLibrary/Joy1.h"

//Snes joy 1 pins
#define SNES1_CLOCK  9
#define SNES1_LATCH  8
#define SNES1_DATA1  7

//Snes joy 2 pins
#define SNES2_CLOCK  A2
#define SNES2_LATCH  A1
#define SNES2_DATA1  A0

//Snes joy 3 pins
#define SNES3_DATA1  6

//Snes joy 4 pins
#define SNES4_DATA1  2

//Snes joy 5 pins
#define SNES5_DATA1  10

//Snes joy 6 pins
#define SNES6_DATA1  3

#ifdef SNES_ENABLE_MULTITAP
  #define SNES1_DATA2  5
  #define SNES1_SELECT 4
  SnesPort<SNES1_CLOCK, SNES1_LATCH, SNES1_DATA1, SNES1_DATA2, SNES1_SELECT> snes;
  SnesPort<SNES2_CLOCK, SNES2_LATCH, SNES2_DATA1> snes2;
  SnesPort<SNES2_CLOCK, SNES2_LATCH, SNES3_DATA1> snes3;
  SnesPort<SNES2_CLOCK, SNES2_LATCH, SNES4_DATA1> snes4;
  SnesPort<SNES2_CLOCK, SNES2_LATCH, SNES5_DATA1> snes5;
  SnesPort<SNES2_CLOCK, SNES2_LATCH, SNES6_DATA1> snes6;
#else
  SnesPort<SNES1_CLOCK, SNES1_LATCH, SNES1_DATA1> snes1;
  SnesPort<SNES2_CLOCK, SNES2_LATCH, SNES2_DATA1> snes2;
  SnesPort<SNES2_CLOCK, SNES2_LATCH, SNES3_DATA1> snes3;
  SnesPort<SNES2_CLOCK, SNES2_LATCH, SNES4_DATA1> snes4;
  SnesPort<SNES2_CLOCK, SNES2_LATCH, SNES5_DATA1> snes5;
  SnesPort<SNES2_CLOCK, SNES2_LATCH, SNES6_DATA1> snes6;
#endif

void snesResetJoyValues(const uint8_t i) {
  if (i >= totalUsb)
    return;

  usbStick[i]->resetState();
}

void snesSetup() {
  //Init the class
  snes1.begin();
  snes2.begin();
  snes3.begin();
  snes4.begin();
  snes5.begin();
  snes6.begin();

  delayMicroseconds(10);

 //Multitap is connected?
  const uint8_t tap = snes.getMultitapPorts();
  if (tap == 0){ //No multitap connected during boot
    totalUsb = 1;
    sleepTime = 50;
  } else { //Multitap connected
    totalUsb = min(tap, MAX_USB_STICKS);
    sleepTime = 1000; //use longer interval between reads for multitap
  }

  //Create usb controllers
  for (uint8_t i = 0; i < totalUsb; i++) {
    usbStick[i] = new Joy1_("RZordSnes", JOYSTICK_DEFAULT_REPORT_ID + i, JOYSTICK_TYPE_GAMEPAD, totalUsb);
  }

  //Set usb parameters and reset to default values
  for (uint8_t i = 0; i < totalUsb; i++) {
      snesResetJoyValues(i);
      usbStick[i]->sendState();
  }
  
  dstart (115200);
}

inline bool __attribute__((always_inline))
snesLoop() {
  static uint8_t lastControllerCount = 0;
  //const unsigned long start = micros();

  //Read snes port
  //It's not required to disable interrupts but it will gain some performance
  noInterrupts();
  snes1.update();
  snes2.update();
  snes3.update();
  snes4.update();
  snes5.update();
  snes6.update();
  interrupts();

  const unsigned long start = micros();
  
  //Get the number of connected controllers
  const uint8_t joyCount = snes.getControllerCount();

  for (uint8_t i = 0; i < joyCount; i++) {
    if (i == totalUsb)
      break;
      
    //Get the data for the specific controller
       const SnesController& sc = (i < joyCount1) ? snes1.getSnesController(i) :
                           (i - joyCount1 < joyCount2) ? snes2.getSnesController(i - joyCount1) :
                           (i - joyCount1 - joyCount2 < joyCount3) ? snes3.getSnesController(i - joyCount1 - joyCount2) :
                           (i - joyCount1 - joyCount2 - joyCount3 < joyCount4) ? snes4.getSnesController(i - joyCount1 - joyCount2 - joyCount3) :
                           (i - joyCount1 - joyCount2 - joyCount3 - joyCount4 < joyCount5) ? snes5.getSnesController(i - joyCount1 - joyCount2 - joyCount3 - joyCount4) :
                           snes6.getSnesController(i - joyCount1 - joyCount2 - joyCount3 - joyCount4 - joyCount5);
    
    //Only process data if state changed from previous read
    if(sc.stateChanged()) {
      //Controller just connected.
      if (sc.deviceJustChanged())
        snesResetJoyValues(i);

      uint8_t hatData = sc.hat();

      if (sc.deviceType() == SNES_DEVICE_NES) {
        ((Joy1_*)usbStick[i])->setButton(1, sc.digitalPressed(SNES_Y));
        ((Joy1_*)usbStick[i])->setButton(2, sc.digitalPressed(SNES_B));
      } else {
        ((Joy1_*)usbStick[i])->setButton(0, sc.digitalPressed(SNES_Y));
        ((Joy1_*)usbStick[i])->setButton(1, sc.digitalPressed(SNES_B));
        ((Joy1_*)usbStick[i])->setButton(2, sc.digitalPressed(SNES_A));
        ((Joy1_*)usbStick[i])->setButton(3, sc.digitalPressed(SNES_X));
        ((Joy1_*)usbStick[i])->setButton(4, sc.digitalPressed(SNES_L));
        ((Joy1_*)usbStick[i])->setButton(5, sc.digitalPressed(SNES_R));
      }
      ((Joy1_*)usbStick[i])->setButton(8, sc.digitalPressed(SNES_SELECT));
      ((Joy1_*)usbStick[i])->setButton(9, sc.digitalPressed(SNES_START));

      //Get angle from hatTable and pass to joystick class
      ((Joy1_*)usbStick[i])->setHatSwitch(hatTable[hatData]);

      usbStick[i]->sendState();
    }
  }

  //Controller has been disconnected? Reset it's values!
  if (lastControllerCount > joyCount) {
    for (uint8_t i = joyCount; i < lastControllerCount; i++) {
      if (i == totalUsb)
        break;
      snesResetJoyValues(i);
      usbStick[i]->sendState();
    }
  }

  //Keep count for next read
  lastControllerCount = joyCount;
  
  //sleep if total loop time was less than sleepTime
  unsigned long delta = micros() - start;
  //debugln(delta);
  if (delta < sleepTime) {
    delta = sleepTime - delta;
    delayMicroseconds(delta);
    //debugln(delta);
  }
  
  return joyCount != 0;
}
