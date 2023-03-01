#include <Arduino.h>
#include <MemoryFree.h>
#include "Logger.hpp"
#include "time.h"

/*

  This logger will perform non-blocking writes to Serial.
  If Serial is busy, the data will be buffered in order to not block.

*/

// 1KiB
#define SIZE_BUFFER 1024

static const char endl[] = "\r\n";
static const size_t endlLength = strlen(endl);

Logger::Logger(arduino::HardwareSerial *serial) : serialPtr(serial) {
  // reserve memory for String used as a buffer
  buffer.reserve(SIZE_BUFFER);
}

/// Transmits the amount of outgoing (buffered) serial data that Serial
/// is capable to write without blocking.
void Logger::writeWhileAvailable() {
  if (!ensureSerial()) return;

  // check amount of data that Serial is capable to write without blocking
  unsigned int serialAvailableForWrite = serialPtr->availableForWrite();
  unsigned int chunkSize = min(buffer.length(), serialAvailableForWrite);

  if (chunkSize > 0) {
    serialPtr->write(buffer.c_str(), chunkSize);
    buffer.remove(0, chunkSize);
  }
}

/// Waits for the transmission of outgoing (buffered) serial data to complete.
void Logger::flush() {
  if (!ensureSerial()) return;

  unsigned int size = buffer.length();

  if (size > 0) {
    serialPtr->write(buffer.c_str(), size);
    buffer.remove(0, size);

    serialPtr->flush();
  }
}

bool Logger::print(const char str[], bool shouldAppendEndl) {  
  if (!str) return false;
  if (!ensureSerial()) return true;

  // new length of buffer
  size_t length = buffer.length() + strlen(str) + (shouldAppendEndl ? endlLength : 0);
  if (length > SIZE_BUFFER) {
    handleBufferOverflow();

    return false;
  }
  
  buffer += str;

  if (shouldAppendEndl) {
    buffer += endl;
  }

  writeWhileAvailable();
  
  return true;
}
bool Logger::print(const String &s, bool shouldAppendEndl) { return print(s.c_str(), shouldAppendEndl); }
bool Logger::print(char c, bool shouldAppendEndl) { return print(String(c).c_str(), shouldAppendEndl); }
bool Logger::print(unsigned char b, bool shouldAppendEndl) { return print(String(b).c_str(), shouldAppendEndl); }
bool Logger::print(int n, bool shouldAppendEndl) { return print(String(n).c_str(), shouldAppendEndl); }
bool Logger::print(unsigned int n, bool shouldAppendEndl) { return print(String(n).c_str(), shouldAppendEndl); }
bool Logger::print(long n, bool shouldAppendEndl) { return print(String(n).c_str(), shouldAppendEndl); }
bool Logger::print(unsigned long n, bool shouldAppendEndl) { return print(String(n).c_str(), shouldAppendEndl); }

bool Logger::print(const char str[]) { return print(str, false); }
bool Logger::print(const String &s) { return print(s, false); }
bool Logger::print(char c) { return print(c, false); }
bool Logger::print(unsigned char b) { return print(b, false); }
bool Logger::print(int n) { return print(n, false); }
bool Logger::print(unsigned int n) { return print(n, false); }
bool Logger::print(long n) { return print(n, false); }
bool Logger::print(unsigned long n) { return print(n, false); }

bool Logger::println(const char str[]) { return print(str, true); }
bool Logger::println(const String &s) { return print(s, true); }
bool Logger::println(int n) { return print(n, true); }
bool Logger::println(unsigned int n) { return print(n, true); }
bool Logger::println(long n) { return print(n, true); }
bool Logger::println(unsigned long n) { return print(n, true); }

void Logger::printTimestamp() {
  print(String(now()) + " ms");
  print(" (free RAM: " + String(freeMemory(), DEC) + ")");  // print how much RAM is available
  print(" -> ");
}

/// Returns `true`, when Serial is available.
bool Logger::ensureSerial() {
    return (bool)serialPtr;
}

void Logger::handleBufferOverflow() {
  unsigned int size = buffer.length();

  serialPtr->println();
  serialPtr->println();
  serialPtr->print("WARNING: Buffer overflow! Logs will be dropped.");
  serialPtr->println(" (" + String(size) + " B).");

  serialPtr->println();
  serialPtr->println();

  // Clear buffer
  buffer.remove(0, size);
}
