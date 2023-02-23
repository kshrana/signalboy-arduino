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
#include <LCDKeypadShieldLib.h>
#include "constants.h"
#include "Logger.hpp"
#include "time.h"
#include "training.h"
#include "IntroViewController.h"
#include "ErrorViewController.h"
#include "MainViewController.h"
#include "Error.h"
#include "Resources.h"

// Uncomment to set Debug-Flag (or pass via compiler-flag)
// #define DEBUG

/// The states that will be displayed to the user
/// using the LCD-display.
enum State_t {
  stateINITIAL,
  // BLE is advertising and connectable.
  stateAWAITING_CONNECTION,
  stateCONNECTED,
};

#define REVISION_STRING_SIZE_BYTES 8
BLEService deviceInformationService("180a");
BLECharacteristic hardwareRevisionChar("2a27", BLERead, REVISION_STRING_SIZE_BYTES, false);
BLECharacteristic softwareRevisionChar("2a28", BLERead, REVISION_STRING_SIZE_BYTES, false);

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

BLEService connectionInformationService("a5210000-9859-499a-ad8a-1264b41a7750");
// OptionSet-value indicating options specific to an established connection.
BLEByteCharacteristic connectionOptionsChar("a5210001-9859-499a-ad8a-1264b41a7750", BLERead | BLENotify);

// pin == HIGH if signal is triggered (either by "Scheduled-timer" or "Trigger-timer") for the duration
// of `SIGNAL_HIGH_INTERVAL`.
const int PIN_OUTPUT = 10;
// INPUT which may trigger activation of the "Trigger-timer" (for DEBUG-purposes)
const int PIN_INPUT_DEBUG = 12;

// Pins used for the LCD-Display
const int PIN_LCD_RS = 7;
const int PIN_LCD_ENABLE = 8;
const int PIN_LCD_D0 = 3;
// CAUTION: Note that we deviate from the sequence here,
// with `PIN_LCD_D1` being set to Pin "D3".
const int PIN_LCD_D1 = 2;
const int PIN_LCD_D2 = 4;
const int PIN_LCD_D3 = 6;

// Set to HIGH to enable the lcd-panel's backlight.
const int PIN_BACKLIGHT = 9;

// #define CONNECTION_INTERVAL_15MS 0x000C
#define CONNECTION_INTERVAL_20MS 0x0010
// #define CONNECTION_INTERVAL_100MS 0x0050

// #define CONNECTION_INTERVAL_DEFAULT CONNECTION_INTERVAL_100MS
#define CONNECTION_INTERVAL_TRAINING CONNECTION_INTERVAL_20MS

#define TIMEOUT_INTRO_SCREEN 3 * 1000UL  // in ms

/// The state that is currently displayed to the user
/// using the LCD-display.
State_t displayedState;

bool isBLESetupComplete = false;

// If `true`, a "heartbeat" is emitted over the gpio-pin every 3 seconds.
// Turned off in production.
#ifdef DEBUG
bool isHeartbeatEnabled = false;

unsigned long loopCount = 0;
float avgLoopRuntime = 0.0;

unsigned long lastPrintEventLoopStatsTime = 0;
#endif

// - Main/"Scheduled Timer"-Timer
bool isScheduledTimerArmed = false;
bool hasScheduledTimerFired = false;
// The timestamp (synced time) at which HIGH output signal will _begin_ to be fired (for the duration of `SIGNAL_HIGH_INTERVAL`)
unsigned long scheduled_timer_fire_timestamp = 0;

// - Alternative/"Trigger"-Timer
bool isTriggerTimerArmed = false;
bool hasTriggerTimerFired = false;
// The timestamp (local-unsynced time) at which HIGH output signal will _begin_ to be fired (for the duration of `SIGNAL_HIGH_INTERVAL`)
unsigned long trigger_timer_fire_timestamp = 0;

/* --- LCD-Display --- */

LCDKeypadScreen screen(
  LCD_NUM_COL,
  PIN_LCD_RS,
  PIN_LCD_ENABLE,
  PIN_LCD_D0,
  PIN_LCD_D1,
  PIN_LCD_D2,
  PIN_LCD_D3,
  PIN_BACKLIGHT);

IntroViewController introViewController;
ErrorViewController errorViewController;
MainViewController mainViewController;

struct RejectConnectionMenuItem : public IMenuItem {
  String getLabel() {
    return Resources::shared->rejectConnection;
  }

  void onSelection() {
    mainViewController.dismissMenu();

    connectionOptionsChar.writeValue(connectionOptionsChar.value() | CONNECTION_OPTION_REJECT_REQUEST);
  }
};

std::unique_ptr<std::vector<std::unique_ptr<IMenuItem>>> makeStateConnectedMenu() {
  std::unique_ptr<RejectConnectionMenuItem> dropConnectionMenuItemPtr(new RejectConnectionMenuItem);

  std::unique_ptr<std::vector<std::unique_ptr<IMenuItem>>> menuItemsPtr(new std::vector<std::unique_ptr<IMenuItem>>);
  menuItemsPtr->push_back(std::move(dropConnectionMenuItemPtr));

  return menuItemsPtr;
}

// MARK: - Private

#ifdef DEBUG
// Only used for debugging-purposes.
void blockThreadUntilSerialOpen() {
  // Inform user via lcd-display.
  introViewController.m_isShowingAwaitingSerialPortNotice = true;

  // Initialize serial and wait for port to open... (needed for native USB port only)
  while (!Serial) {
    screen.update();
  }
}
#endif

void armScheduledTimer(unsigned long timestamp) {
  scheduled_timer_fire_timestamp = timestamp;

  hasScheduledTimerFired = false;
  isScheduledTimerArmed = true;
}

void armTriggerTimer() {
  // timestamp using local unsynced-`millis()` instead of synced-`now()`
  trigger_timer_fire_timestamp = millis();

  hasTriggerTimerFired = false;
  isTriggerTimerArmed = true;
}

bool inputValue = false;
void pollInput() {
  bool newValue = digitalRead(PIN_INPUT_DEBUG);
  if (newValue && !inputValue) {
    Log.printTimestamp();
    Log.println("Rising-edge detected. Arming trigger timer...");
    // rising edge
    armTriggerTimer();
  }

  inputValue = newValue;
}

void updateTimeNeedsSync() {
  bool timeNeedsSync = timeNeedsSyncChar.value();
  bool newValue = timeStatus() != timeSet;

  if (newValue != timeNeedsSync) {
    Log.printTimestamp();
    byte data = newValue ? 0x01 : 0x00;
    Log.print(": update timeNeedsSync-characteristic, new value: ");
    Log.println(data);

    timeNeedsSyncChar.writeValue(data);

    // @WORKAROUND[1]: Set Connection-Interval for Training-Mode initially
    // when setting up the connection.
    //
    // Background: Changing the Connection Interval for an established connection seems
    // not to be supported currently by ArduinoBLE-library.
    // if (newValue) {
    //   Log.println("Will change Connection-Interval for Training-Mode.");
    //   BLE.setConnectionInterval(CONNECTION_INTERVAL_TRAINING, CONNECTION_INTERVAL_TRAINING);
    // } else {
    //   Log.println("Will change Connection-Interval for Default-Mode. (time is synced)");
    //   BLE.setConnectionInterval(CONNECTION_INTERVAL_DEFAULT, CONNECTION_INTERVAL_DEFAULT);
    // }
  }
}

State_t getState() {
  if (BLE.central()) {
    return stateCONNECTED;
  } else if (isBLESetupComplete) {
    return stateAWAITING_CONNECTION;
  }

  return stateINITIAL;
}

/// Updates the state-description displayed to the user
/// (LCD-display).
void updateStateDisplay() {
  State_t state = getState();
  bool isStateChanged = state != displayedState;

  String text = "";
  std::unique_ptr<std::vector<std::unique_ptr<IMenuItem>>> updatedMenuItemsPtr = {};

  switch (state) {
    case stateINITIAL:
      {
        text = Resources::shared->getStateLabel_init();
        if (isStateChanged) {
          updatedMenuItemsPtr.reset(new std::vector<std::unique_ptr<IMenuItem>>());
        }
        break;
      }
    case stateAWAITING_CONNECTION:
      {
        text = Resources::shared->getStateLabel_awaitingConnection();
        if (isStateChanged) {
          updatedMenuItemsPtr.reset(new std::vector<std::unique_ptr<IMenuItem>>());
        }
        break;
      }
    case stateCONNECTED:
      {
        bool isSynced = !timeNeedsSyncChar.value();
        text = Resources::shared->getStateLabel_connected(isSynced);

        if (isStateChanged) {
          auto menuItemsPtr = makeStateConnectedMenu();
          updatedMenuItemsPtr = std::move(menuItemsPtr);
        }
        break;
      }
  }

  mainViewController.setText(text);
  if (updatedMenuItemsPtr.get()) {
    mainViewController.setMenuItems(*updatedMenuItemsPtr);
  }
  displayedState = state;

  if (isStateChanged) {
    mainViewController.dismissMenu();
  }
}

// MARK: - Lifecycle

void setup() {
  pinMode(PIN_OUTPUT, OUTPUT);
  pinMode(PIN_INPUT_DEBUG, INPUT_PULLDOWN);

  screen.setup();
  screen.setRootViewController(&introViewController);
  screen.update();

  Serial.begin(9600);
  Serial1.begin(57600);
  // blockThreadUntilSerialOpen();

#ifdef DEBUG
  // Shorten intro-time during development.
  delay(1000UL);
#else
  delay(TIMEOUT_INTRO_SCREEN);
#endif
  // Dismiss intro-screen after initial intro-timeout.
  updateStateDisplay();
  screen.setRootViewController(&mainViewController);
  screen.update();

  // Time-library
  setSyncInterval(SYNC_INTERVAL);

  // Training
  setTimeProvider(now); // Pass `now` as time provider for training.h

  // begin initialization
  if (!BLE.begin()) {
    Log.println("starting Bluetooth® Low Energy module failed!");

    errorViewController.setError(new Signalboy::Error(ERROR_CODE_BLE_INIT_FAILURE, ERROR_MSG_BLE_INIT_FAILURE));
    screen.setRootViewController(&errorViewController);

    while (1) {
      screen.update();
    }
  }

  // @WORKAROUND[1]: Set Connection-Interval for Training-Mode.
  // Search for `WORKAROUND[1]` for further information.
  // BLE.setConnectionInterval(CONNECTION_INTERVAL_DEFAULT, CONNECTION_INTERVAL_DEFAULT);
  BLE.setConnectionInterval(CONNECTION_INTERVAL_TRAINING, CONNECTION_INTERVAL_TRAINING);
  // set the local name peripheral advertises
  BLE.setLocalName("Signalboy_1");
  // set the UUID for the service this peripheral advertises
  BLE.setAdvertisedService(outputService);

  char revisionStringBuffer[REVISION_STRING_SIZE_BYTES];

  snprintf(revisionStringBuffer, sizeof(revisionStringBuffer), "%d", HARDWARE_REVISION);
  hardwareRevisionChar.writeValue(revisionStringBuffer, strlen(revisionStringBuffer), false);
  deviceInformationService.addCharacteristic(hardwareRevisionChar);

  snprintf(revisionStringBuffer, sizeof(revisionStringBuffer), "%d", SOFTWARE_REVISION);
  softwareRevisionChar.writeValue(revisionStringBuffer, strlen(revisionStringBuffer), false);
  deviceInformationService.addCharacteristic(softwareRevisionChar);

  BLE.addService(deviceInformationService);

  // add the characteristic to the service
  outputService.addCharacteristic(targetTimestampChar);
  outputService.addCharacteristic(triggerTimerChar);
  BLE.addService(outputService);

  timeSyncService.addCharacteristic(timeNeedsSyncChar);
  timeSyncService.addCharacteristic(referenceTimestampChar);
  BLE.addService(timeSyncService);

  connectionInformationService.addCharacteristic(connectionOptionsChar);
  BLE.addService(connectionInformationService);

  // assign event handlers for connected, disconnected to peripheral
  BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

  targetTimestampChar.setEventHandler(BLEWritten, onTargetTimestampWritten);
  targetTimestampChar.writeValue(0);

  triggerTimerChar.setEventHandler(BLEWritten, onTriggerTimerWritten);

  timeNeedsSyncChar.writeValue(1);

  referenceTimestampChar.setEventHandler(BLEWritten, onReferenceTimestampWritten);
  referenceTimestampChar.writeValue(0);

  connectionOptionsChar.writeValue(0);

  // start advertising
  BLE.advertise();

  isBLESetupComplete = true;
  Log.println(("Bluetooth® device active, waiting for connections..."));
}

void loop() {
#ifdef DEBUG
  unsigned long startTime = millis();
#endif
  
  eventLoop();

#ifdef DEBUG
  unsigned long endTime = millis();

  if (endTime - startTime > 2) {
    Log.println("WARNING: Loop took " + String(endTime - startTime) + "ms!");
  }

  updateEventLoopStats(startTime, endTime);

  if (endTime - lastPrintEventLoopStatsTime >= 3000 && endTime != lastPrintEventLoopStatsTime) {
    printLoopRuntimeStats();
    resetRuntimeStats();

    lastPrintEventLoopStatsTime = endTime;
  }
#endif /* DEBUG */
}

void eventLoop() {
  /* --- High Priority --- */

  Log.writeWhileAvailable();

  updateOutputPin();

  setTrainingTimeoutIfNeeded();
  
  // poll for Bluetooth® Low Energy events
  BLE.poll(0);

  // poll for GPIO-input pin (DEBUG)
  pollInput();

  /* --- Low Priority (execution suspended during Training or when any Alarm is armed) --- */
  if (trainingStatus().statusCode == trainingPending || isScheduledTimerArmed || isTriggerTimerArmed) {
    return;
  }

  updateTimeNeedsSync();

  // Handle LCD-display
  updateStateDisplay();
  screen.update();
}

void blePeripheralConnectHandler(BLEDevice central) {
  Log.printTimestamp();

  // central connected event handler
  Log.print("Connected event, central: ");
  Log.println(central.address());

#ifdef DEBUG
  // isHeartbeatEnabled = false;
#endif
}

void blePeripheralDisconnectHandler(BLEDevice central) {
  Log.printTimestamp();

  // central disconnected event handler
  Log.print("Disconnected event, central: ");
  Log.println(central.address());

  // Reset connection options.
  connectionOptionsChar.writeValue(0);

#ifdef DEBUG
  // isHeartbeatEnabled = true;
#endif
}

bool isScheduledTimerDue() {
  unsigned long nowTime = now();
  unsigned long targetTime = targetTimestampChar.value();

  if (isScheduledTimerArmed) {
    return nowTime >= targetTime && nowTime < targetTime + SIGNAL_HIGH_INTERVAL;
  }

  return false;
}

bool isTriggerTimerDue() {
  unsigned long nowTimeUnsynced = millis();
  
  if (isTriggerTimerArmed) {
    return nowTimeUnsynced >= trigger_timer_fire_timestamp && nowTimeUnsynced < trigger_timer_fire_timestamp + /* FIXME: SIGNAL_HIGH_INTERVAL */ 10;
  }

  return false;
}

void updateOutputPin() {
  PinStatus value = LOW;

  if (isScheduledTimerDue()) {
    if (!hasScheduledTimerFired) {
      Log.printTimestamp();
      Log.println("Fire! (Scheduled Timer)");

      hasScheduledTimerFired = true;
    }

    // Turn on
    value = HIGH;
  } else {
    if (hasScheduledTimerFired) {
      isScheduledTimerArmed = false;
    }
  }

  if (isTriggerTimerDue()) {
    if (!hasTriggerTimerFired) {
      Log.printTimestamp();
      Log.println("Fire! (Trigger Timer)");

      hasTriggerTimerFired = true;
    }

    // Turn on
    value = HIGH;
  } else {
    if (hasTriggerTimerFired) {
      isTriggerTimerArmed = false;
    }
  }

#ifdef DEBUG
  // Heartbeat:
  // Heartbeat is emitted every 3 seconds.
  if (isHeartbeatEnabled && now() % 3000 <= SIGNAL_HIGH_INTERVAL) {
    value = HIGH;
  }
#endif

  // Finally write (signal-)value to output
  // if (value == HIGH) Log.println("HIGH");
  digitalWrite(PIN_OUTPUT, value);
}

void onTargetTimestampWritten(BLEDevice central, BLECharacteristic characteristic) {
  Log.printTimestamp();

  // central wrote new value to characteristic
  Log.print("on -> Characteristic event (targetTimestamp), value: ");

  unsigned long value = targetTimestampChar.value();
  Log.print(value);

  Log.print(", delta: ");
  Log.println(value - now());

  armScheduledTimer(value);
}

void onTriggerTimerWritten(BLEDevice central, BLECharacteristic characteristic) {
  Log.printTimestamp();

  // central wrote new value to characteristic
  Log.print("on -> Characteristic event (triggerOutput), value: ");

  byte value = triggerTimerChar.value();
  Log.println(value);

  armTriggerTimer();
}

void onReferenceTimestampWritten(BLEDevice central, BLECharacteristic characteristic) {
  unsigned long receivedTime = now();

  // central wrote new value to characteristic
  Log.print(receivedTime);
  Log.print(" ms -> ");
  Log.print("on -> Characteristic event (referenceTimestamp), written: ");

  unsigned long value = referenceTimestampChar.value();
  Log.println(value);

  onReceivedReferenceTimestamp(receivedTime, value);
  TrainingStatus status = trainingStatus();

  switch (status.statusCode) {
    case trainingSucceeded:
      Log.print("Training succeeded. Setting time with synced timestamp (adjusted by network delay): ");
      Log.println(status.adjustedReferenceTimestamp);

      setTime(status.adjustedReferenceTimestamp);
      updateTimeNeedsSync();
      break;

    default:
      break;
  }
}

#ifdef DEBUG
void resetRuntimeStats() {
  avgLoopRuntime = 0.0;
  loopCount = 0;
}

void updateEventLoopStats(unsigned long startTime, unsigned long endTime) {  
  avgLoopRuntime = (avgLoopRuntime * loopCount + (endTime - startTime)) / (loopCount + 1);

  loopCount++;
}

char _buffer [20];
void printLoopRuntimeStats() {
  Log.printTimestamp();
  snprintf(_buffer, 20, "%.2f", avgLoopRuntime);
  Log.println("avgLoopRuntime=" + String(_buffer));
}
#endif
