#include <Arduino.h>
#include "logger.h"
#include "time.h"

void printTimestamp() {
  Serial.print(now());
  Serial.print(" ms -> ");
}
