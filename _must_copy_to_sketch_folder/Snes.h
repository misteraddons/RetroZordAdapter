/*******************************************************************************
 * Snes controller input library.
 * https://github.com/sonik-br/SnesLib
*/

#include "src/SnesLib/SnesLib.h"

//Snes joy 1 pins
#define SNES1_CLOCK  9
#define SNES1_LATCH  8
#define SNES1_DATA1  7

//Snes joy 2 pins
#define SNES2_CLOCK  A2
#define SNES2_LATCH  A1
#define SNES2_DATA1  A0

//Snes joy 3 pins
#define SNES3_CLOCK  15
#define SNES3_LATCH  3
#define SNES3_DATA1  6

//Snes joy 4 pins
#define SNES4_CLOCK  14
#define SNES4_LATCH  16
#define SNES4_DATA1  10

#ifdef SNES_ENABLE_MULTITAP
  #define SNES1_DATA2  5
  #define SNES1_SELECT 3
  SnesPort<SNES1_CLOCK, SNES1_LATCH, SNES1_DATA1, SNES1_DATA2, SNES1_SELECT> snes;
  SnesPort<SNES2_CLOCK, SNES2_LATCH, SNES2_DATA1> snes2;
  SnesPort<SNES3_CLOCK, SNES3_LATCH, SNES3_DATA1> snes3;
  SnesPort<SNES4_CLOCK, SNES4_LATCH, SNES4_DATA1> snes4;
#else
  SnesPort<SNES1_CLOCK, SNES1_LATCH, SNES1_DATA1> snes1;
  SnesPort<SNES2_CLOCK, SNES2_LATCH, SNES2_DATA1> snes2;
  SnesPort<SNES3_CLOCK, SNES3_LATCH, SNES3_DATA1> snes3;
  SnesPort<SNES4_CLOCK, SNES4_LATCH, SNES4_DATA1> snes4;
#endif

#define SNES_USB_BUTTON_COUNT 10


void snesResetJoyValues(const uint8_t i) {
  if (i >= totalUsb)
    return;
    
  for (uint8_t x = 0; x < SNES_USB_BUTTON_COUNT; x++)
    usbStick[i]->releaseButton(x);
}

void snesSetup() {

  //Init the class
  snes1.begin();
  snes2.begin();
  snes3.begin();
  snes4.begin();


  delayMicroseconds(10);

  //Multitap is connected?
  const uint8_t tap = snes1.getMultitapPorts();
  if (tap == 0){ //No multitap connected during boot
    totalUsb = 4;
    //sleepTime = 500;
    sleepTime = 100;
  } else { //Multitap connected
    totalUsb = min(tap, MAX_USB_STICKS);
    sleepTime = 1000; //use longer interval between reads for multitap
  }
  
  for (uint8_t i = 0; i < totalUsb; i++) {
    usbStick[i] = new Joystick_ (
      "RZordSnes",
      JOYSTICK_DEFAULT_REPORT_ID + i,
      JOYSTICK_TYPE_GAMEPAD,
      SNES_USB_BUTTON_COUNT      // buttonCount
    );
    usbStick[i]->begin (false);
  }
  
  dstart (115200);
}

inline void __attribute__((always_inline))
snesLoop() {
  static uint8_t lastControllerCount = 0;
  const unsigned long start = micros();

  //Read snes port
  //It's not required to disable interrupts but it will gain some performance
  noInterrupts();
  snes1.update();
  snes2.update();
  snes3.update();
  snes4.update();
  interrupts();
  
  //Get the number of connected controllers
  const uint8_t joyCount1 = snes1.getControllerCount();
  const uint8_t joyCount2 = snes2.getControllerCount();
  const uint8_t joyCount3 = snes3.getControllerCount();
  const uint8_t joyCount4 = snes4.getControllerCount();
  const uint8_t joyCount = joyCount1 + joyCount2 + joyCount3 + joyCount4;
  Serial.println(joyCount);

  for (uint8_t i = 0; i < joyCount; i++) {
    if (i == totalUsb)
      break;
      
    //Get the data for the specific controller
//        const SnesController& sc = (i < joyCount1) ? snes1.getSnesController(i) : snes2.getSnesController(i - joyCount1);
        const SnesController& sc = (i < joyCount1) ? snes1.getSnesController(i) :
                           (i - joyCount1 < joyCount2) ? snes2.getSnesController(i - joyCount1) :
                           (i - joyCount1 - joyCount2 < joyCount3) ? snes3.getSnesController(i - joyCount1 - joyCount2) :
                           snes4.getSnesController(i - joyCount1 - joyCount2 - joyCount3);
                           
    //Only process data if state changed from previous read
    if(sc.stateChanged()) {
      //Controller just connected.
      if (sc.deviceJustChanged())
        snesResetJoyValues(i);

      uint8_t hatData = sc.hat();

      if (sc.deviceType() == SNES_DEVICE_NES) {
        usbStick[i]->setButton(1, sc.digitalPressed(SNES_Y));
        usbStick[i]->setButton(2, sc.digitalPressed(SNES_B));
      } else {
        usbStick[i]->setButton(0, sc.digitalPressed(SNES_Y));
        usbStick[i]->setButton(1, sc.digitalPressed(SNES_B));
        usbStick[i]->setButton(2, sc.digitalPressed(SNES_A));
        usbStick[i]->setButton(3, sc.digitalPressed(SNES_X));
        usbStick[i]->setButton(4, sc.digitalPressed(SNES_L));
        usbStick[i]->setButton(5, sc.digitalPressed(SNES_R));
      }
      usbStick[i]->setButton(8, sc.digitalPressed(SNES_SELECT));
      usbStick[i]->setButton(9, sc.digitalPressed(SNES_START));

      //Get angle from hatTable and pass to joystick class
      usbStick[i]->setHatSwitch(0, hatTable[hatData]);

      usbStick[i]->sendState();
    }
  }

  //Controller has been disconnected? Reset it's values!
  if (lastControllerCount > joyCount) {
    for (uint8_t i = joyCount; i < lastControllerCount; i++) {
      if (i == totalUsb)
        break;
      snesResetJoyValues(i);
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

  if (joyCount == 0) //blink led while no controller connected
    blinkLed();
}
