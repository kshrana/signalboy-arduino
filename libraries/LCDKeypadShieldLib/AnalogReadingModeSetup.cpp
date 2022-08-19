#include <Arduino.h>

/// `true`, if analog reading mode has been set.
bool isAnalogReadingModeSet = false;

void setupAnalogReadingFor5vAREF() {
  uint8_t analogReadingMode =
    AR_EXTERNAL;  // Arduino SAMD Boards (Zero, etc.)
  // s. reference for other accepted values:
  // https://www.arduino.cc/reference/en/language/functions/analog-io/analogreference/
  analogReference(analogReadingMode);
  isAnalogReadingModeSet = true;
}

// Before calling Arduino's analogRead-method, we have to make sure
// that we have set the analog reading mode to EXTERNAL, or else we
// might damage the microcontroller, s. "Notes and Warnings" in Arduino-
// reference: https://www.arduino.cc/reference/en/language/functions/analog-io/analogreference/
int analogReadIfSafe(pin_size_t pinNumber) {
  if (!isAnalogReadingModeSet) {
    // return max value for analogRead(), s. docs:
    // https://www.arduino.cc/reference/en/language/functions/analog-io/analogread/
    return 1023;
  }
  return analogRead(pinNumber);
}
