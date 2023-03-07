#pragma once

#define HARDWARE_REVISION 1
#define SOFTWARE_REVISION 1

const unsigned long CONNECTION_INTERVAL = 20UL;    // 20 ms
const unsigned long SIGNAL_HIGH_INTERVAL = 100UL;  // 100 ms

const unsigned long SYNC_INTERVAL = 90000UL;  // 90 sec
const int TRAINING_MSGS_COUNT = 3;

#define LCD_NUM_COL 16

/// A number of options that may be indicated by the Peripheral's
/// `connectionOptions`-Characteristic in regards to an established
/// connection.
enum ConnectionOptions {
  /// Indicates that the Peripheral (Signalboy-Device) wants
  /// the Central (client-device, i.e. Meta Quest device) to
  /// disconnect and suppress any reconnect-attempts for a
  /// certain duration.
  CONNECTION_OPTION_REJECT_REQUEST = 1 << 0,
};