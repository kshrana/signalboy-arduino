/*
  Callback LED

  This example creates a Bluetooth® Low Energy peripheral with service that contains a
  characteristic to control an LED. The callback features of the
  library are used.

  The circuit:
  - Arduino MKR WiFi 1010, Arduino Uno WiFi Rev2 board, Arduino Nano 33 IoT,
    Arduino Nano 33 BLE, or Arduino Nano 33 BLE Sense board.

  You can use a generic Bluetooth® Low Energy central app, like LightBlue (iOS and Android) or
  nRF Connect (Android), to interact with the services and characteristics
  created in this sketch.

  This example code is in the public domain.
*/

#include <ArduinoBLE.h>
#include "constants.h"
#include "time.h"
#include "training.h"
#include "logger.h"

BLEService deviceInformationService("180a");
BLECharacteristic hardwareRevisionChar("2a27", BLERead, "1");
BLECharacteristic softwareRevisionChar("2a28", BLERead, "1");

BLEService outputService("37410000-b4d1-f445-aa29-989ea26dc614");
// create targetTimestamp (signal) characteristic and allow remote device to read and write
BLEUnsignedLongCharacteristic targetTimestampChar("37410001-b4d1-f445-aa29-989ea26dc614", BLERead | BLEWrite | BLEWriteWithoutResponse);
// create triggerOutput (signal) characteristic and allow remote device to write (trigger)
BLEByteCharacteristic triggerTimerChar("37410002-b4d1-f445-aa29-989ea26dc614", BLEWrite | BLEWriteWithoutResponse);

BLEService timeSyncService("92360000-7858-41a5-b0cc-942dd4189715");
// create switch characteristic ("timeNeedsSync")
BLEByteCharacteristic timeNeedsSyncChar("92360001-7858-41a5-b0cc-942dd4189715", BLERead | BLENotify);
// create unsigned long characteristic ("referenceTimestamp")
BLEUnsignedLongCharacteristic referenceTimestampChar("92360002-7858-41a5-b0cc-942dd4189715", BLEWrite | BLEWriteWithoutResponse);


// const int PIN_1 = 2;  // pin == HIGH, when observed characteristic is `0x01`
// const int PIN_2 = 3;  // pin == HIGH, when observed characteristic is `0x02`
const int PIN_3 = 4;  // pin == HIGH, when observed characteristic is `0x03`
const int PIN_4 = 5;  // INPUT which may trigger activation of the "Trigger-timer" (for DEBUG-purposes)

// #define CONNECTION_INTERVAL_15MS 0x000C
#define CONNECTION_INTERVAL_20MS 0x0010
// #define CONNECTION_INTERVAL_100MS 0x0050

// #define CONNECTION_INTERVAL_DEFAULT CONNECTION_INTERVAL_100MS
#define CONNECTION_INTERVAL_TRAINING CONNECTION_INTERVAL_20MS

// If `true`, a "heartbeat" is emitted over the gpio-pin every 3 seconds.
// Turned off in production.
#ifdef DEBUG
bool enableHeartbeat = true;
#endif

// - Main/"Scheduled Timer"-Timer
bool is_scheduled_timer_valid = false;
bool is_scheduled_timer_fired = false;
// The timestamp (synced time) at which HIGH output signal will _begin_ to be fired (for the duration of `SIGNAL_HIGH_INTERVAL`)
unsigned long scheduled_timer_fire_timestamp = 0;

// - Alternative/"Trigger"-Timer
bool is_trigger_timer_valid = false;
bool is_trigger_timer_fired = false;
// The timestamp (local-unsynced time) at which HIGH output signal will _begin_ to be fired (for the duration of `SIGNAL_HIGH_INTERVAL`)
unsigned long trigger_timer_fire_timestamp = 0;

// MARK: - Private

void armScheduledTimer(unsigned long timestamp) {
  // timestamp using local unsynced-`millis()` instead of synced-`now()`
  scheduled_timer_fire_timestamp = timestamp;
  
  is_scheduled_timer_fired = false;
  is_scheduled_timer_valid = true;
}

void armTriggerTimer() {
  // timestamp using local unsynced-`millis()` instead of synced-`now()`
  trigger_timer_fire_timestamp = millis();
  
  is_trigger_timer_fired = false;
  is_trigger_timer_valid = true;
}

bool inputValue = false;
void pollInput() {
  bool newValue = digitalRead(PIN_4);
  if (newValue && !inputValue) {
    printTimestamp();
    Serial.println("Rising-edge detected. Arming trigger timer...");
    // rising edge
    armTriggerTimer();
  }

  inputValue = newValue;
}

void updateTimeNeedsSync() {
  bool timeNeedsSync = timeNeedsSyncChar.value();
  bool newValue = timeStatus() != timeSet;

  if (newValue != timeNeedsSync) {
    printTimestamp();
    byte data = newValue ? 0x01 : 0x00;
    Serial.print(": update timeNeedsSync-characteristic, new value: ");
    Serial.println(data);

    timeNeedsSyncChar.writeValue(data);

    // @WORKAROUND[1]: Set Connection-Interval for Training-Mode initially
    // when setting up the connection.
    //
    // Background: Changing the Connection Interval for an established connection seems
    // to be not supported currently with ArduinoBLE-library.
    // if (newValue) {
    //   Serial.println("Will change Connection-Interval for Training-Mode.");
    //   BLE.setConnectionInterval(CONNECTION_INTERVAL_TRAINING, CONNECTION_INTERVAL_TRAINING);
    // } else {
    //   Serial.println("Will change Connection-Interval for Default-Mode. (time is synced)");
    //   BLE.setConnectionInterval(CONNECTION_INTERVAL_DEFAULT, CONNECTION_INTERVAL_DEFAULT);
    // }
  }
}

// MARK: - Lifecycle

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // pinMode(PIN_1, OUTPUT);
  // pinMode(PIN_2, OUTPUT);
  pinMode(PIN_3, OUTPUT);
  pinMode(PIN_4, INPUT_PULLDOWN);

  setSyncInterval(SYNC_INTERVAL);

  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");

    while (1);
  }

  // @WORKAROUND[1]: Set Connection-Interval for Training-Mode.
  // Search for `WORKAROUND[1]` for further information.
  // BLE.setConnectionInterval(CONNECTION_INTERVAL_DEFAULT, CONNECTION_INTERVAL_DEFAULT);
  BLE.setConnectionInterval(CONNECTION_INTERVAL_TRAINING, CONNECTION_INTERVAL_TRAINING);
  // set the local name peripheral advertises
  BLE.setLocalName("LEDCallback");
  // set the UUID for the service this peripheral advertises
  BLE.setAdvertisedService(outputService);

  deviceInformationService.addCharacteristic(hardwareRevisionChar);
  deviceInformationService.addCharacteristic(softwareRevisionChar);
  BLE.addService(deviceInformationService);

  // add the characteristic to the service
  outputService.addCharacteristic(targetTimestampChar);
  outputService.addCharacteristic(triggerTimerChar);
  BLE.addService(outputService);

  timeSyncService.addCharacteristic(timeNeedsSyncChar);
  timeSyncService.addCharacteristic(referenceTimestampChar);
  BLE.addService(timeSyncService);

  // assign event handlers for connected, disconnected to peripheral
  BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

  targetTimestampChar.setEventHandler(BLEWritten, onTargetTimestampWritten);
  targetTimestampChar.setValue(0);

  triggerTimerChar.setEventHandler(BLEWritten, onTriggerTimerWritten);

  timeNeedsSyncChar.setValue(1);

  referenceTimestampChar.setEventHandler(BLEWritten, onReferenceTimestampWritten);
  referenceTimestampChar.setValue(0);

  // start advertising
  BLE.advertise();

  Serial.println(("Bluetooth® device active, waiting for connections..."));
}

// unsigned int prevPrintTime = 0;

void loop() {
  // poll for Bluetooth® Low Energy events
  BLE.poll();

  // poll for GPIO-input pin (DEBUG)
  pollInput();

  setTrainingTimeoutIfNeeded();
  updateTimeNeedsSync();

  unsigned int nowTimeUnsynced = millis();  // local unsynced system-time
  unsigned int nowTime = now();
  unsigned long targetTime = targetTimestampChar.value();

  PinStatus value = LOW;
  if (is_scheduled_timer_valid) {
    if (nowTime >= targetTime && nowTime < targetTime + SIGNAL_HIGH_INTERVAL) {
      if (!is_scheduled_timer_fired) {
        printTimestamp();
        Serial.println("Fire! (Scheduled Timer)");
      }
      is_scheduled_timer_fired = true;
      // Turn on
      value = HIGH;
    } else {
      if (is_scheduled_timer_fired) {
        is_scheduled_timer_valid = false;
      }
    }
  }
  
  // Also check if alternative "trigger"-mechanism applies:
  if (is_trigger_timer_valid) {
    if (nowTimeUnsynced >= trigger_timer_fire_timestamp && nowTimeUnsynced < trigger_timer_fire_timestamp + SIGNAL_HIGH_INTERVAL) {
      if (!is_trigger_timer_fired) {
        printTimestamp();
        Serial.println("Fire! (Trigger Timer)");
      }
      is_trigger_timer_fired = true;
      // Turn on
      value = HIGH;
    } else {
      if (is_trigger_timer_fired) {
        is_trigger_timer_valid = false;
      }
    }
  }

#ifdef DEBUG
  // Heartbeat:
  // Heartbeat is emitted every 3 seconds.
  if (enableHeartbeat && nowTime % 3000 <= SIGNAL_HIGH_INTERVAL) {
  // if (isSendingHeartbeat && nowTime % 3000 == 0) {
    value = HIGH;
  }
#endif

  // Finally write value to output
  // if (value == HIGH) Serial.println("HIGH");
  digitalWrite(PIN_3, value);
}

void blePeripheralConnectHandler(BLEDevice central) {
  printTimestamp();
  
  // central connected event handler
  Serial.print("Connected event, central: ");
  Serial.println(central.address());

#ifdef DEBUG
  enableHeartbeat = false;
#endif
}

void blePeripheralDisconnectHandler(BLEDevice central) {
  printTimestamp();
  
  // central disconnected event handler
  Serial.print("Disconnected event, central: ");
  Serial.println(central.address());

#ifdef DEBUG
  enableHeartbeat = true;
#endif
}

void onTargetTimestampWritten(BLEDevice central, BLECharacteristic characteristic) {
  printTimestamp();

  // central wrote new value to characteristic
  Serial.print("on -> Characteristic event (targetTimestamp), value: ");

  unsigned long value = targetTimestampChar.value();
  Serial.print(value);

  Serial.print(" , delta: ");
  Serial.println(value - now());

  armScheduledTimer(value);
}

void onTriggerTimerWritten(BLEDevice central, BLECharacteristic characteristic) {
  printTimestamp();

  // central wrote new value to characteristic
  Serial.print("on -> Characteristic event (triggerOutput), value: ");

  byte value = triggerTimerChar.value();
  Serial.println(value);

  armTriggerTimer();
}

void onReferenceTimestampWritten(BLEDevice central, BLECharacteristic characteristic) {
  unsigned long nowTimeUnsynced = millis();

  printTimestamp();
  
  // central wrote new value to characteristic
  Serial.print("on -> Characteristic event (referenceTimestamp), written: ");

  unsigned long value = referenceTimestampChar.value();
  Serial.println(value);

  onReceivedReferenceTimestamp(value);
  TrainingStatus status = trainingStatus();
  
  switch (status.statusCode) {
    case trainingSucceeded:
      Serial.print("Training succeeded. Setting time with synced timestamp (adjusted by network delay): ");
      Serial.println(status.adjustedReferenceTimestamp);

      setTime(status.adjustedReferenceTimestamp);
      updateTimeNeedsSync();
      break;

    default:
      break;
  }
}
