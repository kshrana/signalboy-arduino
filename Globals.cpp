#include <Arduino.h>
#include "Globals.hpp"
#include "Logger.hpp"

// Logger Serial output is disabled for Release-Builds.
#ifdef DEBUG
arduino::HardwareSerial *const SERIAL_PTR = &Serial1;
#else
arduino::HardwareSerial *const SERIAL_PTR = nullptr;
#endif

Logger Log(SERIAL_PTR);
