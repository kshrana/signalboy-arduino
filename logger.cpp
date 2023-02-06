#include <Arduino.h>
#include <MemoryFree.h>
#include "logger.h"
#include "time.h"

void printTimestamp() {
  Serial.print(now());
  Serial.print(" ms");

  Serial.print("(free RAM: ");
  Serial.print(freeMemory(), DEC);  // print how much RAM is available.
  Serial.print(") -> ");
}
