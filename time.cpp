// The following code is copied and modified from:
// https://github.com/PaulStoffregen/Time/blob/master/Time.cpp

#include <Arduino.h>
#include "Logger.hpp"
#include "time.h"
#include "rtc.hpp"

static unsigned long syncInterval = 300 * MILISECS_PER_SEC;  // time sync will be attempted after this many miliseconds

static unsigned long sysTime = 0;
static unsigned long prevMillisRtc = 0;

static unsigned long nextSyncTime = 0;
static timeStatus_t Status = timeNotSet;

unsigned long now() {
  // calculate number of seconds passed since last call to now()
  while (millisRtc(false) - prevMillisRtc > 0) {
    // millis() and prevMillis are both unsigned ints thus the subtraction will always be the absolute value of the difference
    sysTime++;
    prevMillisRtc++;
  }
  if (nextSyncTime <= sysTime) {
    // if (getTimePtr != 0) {
    //   unsigned long t = getTimePtr();
    //   if (t != 0) {
    //     setTime(t);
    //   } else {
        nextSyncTime = sysTime + syncInterval;
        Status = (Status == timeNotSet) ? timeNotSet : timeNeedsSync;
    //   }
    // }
  }
  return (unsigned long)sysTime;
}

void setTime(unsigned long t) {
  sysTime = (unsigned long)t;
  nextSyncTime = (unsigned long)t + syncInterval;
  Status = timeSet;
  prevMillisRtc = millisRtc(false);  // restart counting from now (thanks to Korman for this fix)
}


// indicates if time has been set and recently synchronized
timeStatus_t timeStatus() {
  now(); // required to actually update the status
  return Status;
}

void setSyncInterval(unsigned long interval) { // set the number of miliseconds between re-sync
  syncInterval = interval;
  nextSyncTime = sysTime + syncInterval;
}
