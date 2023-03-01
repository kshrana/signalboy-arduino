#include <RTClib.h>

#include "rtc.hpp"
#include "Logger.hpp"

RTC_DS3231 rtc;

#define ERROR_FRACT_INC (1024 % 1000)

unsigned long _millis = 0;
unsigned int errorFract = 0;
// counter; set in interrupt callback
volatile unsigned int tickTock = 0;
unsigned int lastTickTock = 0;

void printSqwMode() {
  Ds3231SqwPinMode mode = rtc.readSqwPinMode();

  Log.print("Sqw Pin Mode: ");
  switch (mode) {
    case DS3231_OFF: Log.println("OFF"); break;
    case DS3231_SquareWave1Hz: Log.println("1Hz"); break;
    case DS3231_SquareWave1kHz: Log.println("1.024kHz"); break;
    case DS3231_SquareWave4kHz: Log.println("4.096kHz"); break;
    case DS3231_SquareWave8kHz: Log.println("8.192kHz"); break;
    default: Log.println("UNKNOWN"); break;
  }
}

// INT0 interrupt callback
void pps_tick(void) {
  tickTock++;
}

void setupRtc() {
  if (!rtc.begin()) {
    Log.println("Couldn't find RTC");
    Log.flush();
    while (1) delay(10);
  }

  if (rtc.lostPower()) {
    Log.println("RTC lost power, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  rtc.writeSqwPinMode(DS3231_SquareWave1kHz);  // 1.024kHz
  printSqwMode();
}

unsigned long millisRtc(bool skipSuspendInterrupts) {
  unsigned int tickTockCopy;

  if (skipSuspendInterrupts) {
    // This code path is useful, when called from an context, where
    // interrupts are already suspended (like an ISR).
    tickTockCopy = tickTock;  // capture value (volatile variable)
  } else {
    noInterrupts();
    tickTockCopy = tickTock;  // capture value (volatile variable)
    interrupts();
  }

  while (tickTockCopy - lastTickTock > 0) {
    _millis++;
    lastTickTock++;
    errorFract += ERROR_FRACT_INC;

    // Apply correction (SQW's frequency is set to 1024Hz).
    if (errorFract >= 1024) {
      _millis--;
      errorFract -= 1024;
    }
  }

  return _millis;
}
