#include <ArduinoBLE.h>

/*
  This example:
    - establishes BLE-connection with Signalboy-device
    - simluates event:
      - prints the current time to serial (event-timestamp)
      - sends a "Trigger-Timestamp" via BLE (event-timestamp + normalization-delay)
      - prints the timestamp of receiving HIGH to serial

  The circuit:
    - Arduino Nano 33 IoT (arduino-signalboy-test)
    - Signalboy (arduino-signalboy)
*/

#define PIN_INPUT 2  // Pin, that supports interrupts

#define UUID_SERVICE_OUTPUT "37410000-b4d1-f445-aa29-989ea26dc614"
#define UUID_CHARACTERISTIC_TARGET_TIMESTAMP "37410001-b4d1-f445-aa29-989ea26dc614"
#define UUID_CHARACTERISTIC_TRIGGER_TIMER "37410002-b4d1-f445-aa29-989ea26dc614"

#define UUID_SERVICE_TIME_SYNC "92360000-7858-41a5-b0cc-942dd4189715"
#define UUID_CHARACTERISTIC_TIME_NEEDS_SYNC "92360001-7858-41a5-b0cc-942dd4189715"
#define UUID_CHARACTERISTIC_REFERENCE_TIMESTAMP "92360002-7858-41a5-b0cc-942dd4189715"

#define TRAINING_MSGS_COUNT 3
#define TRAINING_INTERVAL_IN_MILLIS 20UL
#define INTERVAL_SIGNAL_HIGH 500UL
// The fixed-delay that the signaling of an event is postponed by the Signalboy-device. (in ms)
#define DELAY_NORMALIZATION 100UL

unsigned long events[] = {
  0,
  2636,
  16593,
  22697,
  37201,
  42806,
  53990,
  66746,
  71815,
  86512,
  99108,
  105384,
  110222,
  112566,
  123845,
  127284,
  139779,
  143227,
  155316,
  168650,
  170152,
  176707,
  182352,
  186214,
  195817,
  200168,
  208167,
  214928,
  228023,
  230731,
  241363,
  242873,
  246483,
  250391,
  260160,
  263611,
  274735,
  283307,
  286282,
  294852,
  307478,
  309097,
  314745,
  325053,
  332576,
  347291,
  354358,
  367266,
  369618,
  378064,
  385871,
  387464,
  399811,
  411088,
  424817,
  427830,
  436170,
  442189,
  446269,
  448973,
  456240,
  457602,
  463373,
  472280,
  475809,
  490652,
  500595,
  504589,
  507890,
  517060,
  523154,
  530754,
  532171,
  542370,
  545049,
  546688,
  561395,
  571773,
  578339,
  582978,
  590843,
  603597,
};

unsigned long sysTime = 0;
unsigned long lastMillis = 0;

unsigned long startSysTime = 0;

/// `true`, after signal was received via input-pin in the background (via interrupt).
volatile bool hasReceivedSignal = false;
/// The last time the input-pin was set to HIGH.
volatile unsigned long lastRisingEdgeTime = 0;
volatile int lastInputState = 0;

/// The last last time the input-pin was set to HIGH.
unsigned long lastLastRisingEdgeTime = 0;

/// Index of currently active event. The event is either upcoming (scheduled), or a reply for a previous event is being awaited.
int event = 0;

int eventsSentCount = 0;
int eventsReceivedCount = 0;

/// `true`, once all events have been sent.
bool isEveryEventSent = false;

// refers to Signalboy's "TimeNeedsSync"-Characteristic.
bool isSyncNeeded;
unsigned long lastSyncNeededTime = 0;

void setup() {
  pinMode(PIN_INPUT, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_INPUT), pollInput, CHANGE);

  Serial.begin(9600);
  while (!Serial);
  delay(1000);

  // Establish BLE connection.
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");

    while (true)
      ;
  }
  Serial.println("Bluetooth® Low Energy Central - Arduino Events Emitter (Test #5)");

  prompt("Press ENTER to enter main-loop...");

  // start scanning for peripherals
  BLE.scanForUuid(UUID_SERVICE_OUTPUT);
}

void loop() {
  // check whether a peripheral has been discovered
  BLEDevice peripheral = BLE.available();
  if (peripheral) {
    // discovered a peripheral
    Serial.println("Discovered a peripheral");
    Serial.println("-----------------------");
    printDevice(peripheral);
    Serial.println();

    if (!isSignalboyDevice(peripheral)) {
      return;
    }

    BLE.stopScan();

    // TODO: BLE.setConnectionInterval()?

    controlSignalboy(peripheral);

    // peripheral disconnected, start scanning again
    startSysTime = now();
    BLE.scanForUuid(UUID_SERVICE_OUTPUT);
  } else {
    handleInputSignalIfNeeded();
  }
}

void controlSignalboy(BLEDevice peripheral) {
  Serial.println("Connecting...");

  if (peripheral.connect()) {
    Serial.println("Connected");
  } else {
    Serial.println("Failed to connect!");
    return;
  }

  // discover peripheral attributes
  Serial.println("Discovering attributes ...");
  if (peripheral.discoverAttributes()) {
    Serial.println("Attributes discovered");
  } else {
    Serial.println("Attribute discovery failed!");
    peripheral.disconnect();
    return;
  }

  // retrieve the Target-Timestamp characteristic
  BLECharacteristic targetTimestampChar = peripheral.characteristic(UUID_CHARACTERISTIC_TARGET_TIMESTAMP);

  if (!targetTimestampChar) {
    Serial.println("Peripheral does not have Target-Timestamp characteristic!");
    peripheral.disconnect();
    return;
  } else if (!targetTimestampChar.canWrite()) {
    Serial.println("Peripheral does not have a writable Target-Timestamp characteristic!");
    peripheral.disconnect();
    return;
  }

  // retrieve the Trigger characteristic
  BLECharacteristic triggerTimerChar = peripheral.characteristic(UUID_CHARACTERISTIC_TRIGGER_TIMER);

  if (!triggerTimerChar) {
    Serial.println("Peripheral does not have Trigger characteristic!");
    peripheral.disconnect();
    return;
  } else if (!triggerTimerChar.canWrite()) {
    Serial.println("Peripheral does not have a writable Trigger characteristic!");
    peripheral.disconnect();
    return;
  }

  // retrieve the TimeNeedsSync characteristic
  BLECharacteristic timeNeedsSyncChar = peripheral.characteristic(UUID_CHARACTERISTIC_TIME_NEEDS_SYNC);

  if (!timeNeedsSyncChar) {
    Serial.println("Peripheral does not have TimeNeedsSync characteristic!");
    peripheral.disconnect();
    return;
  } else if (!timeNeedsSyncChar.canRead() || !timeNeedsSyncChar.canSubscribe()) {
    Serial.println("Peripheral does not have a readable (or subscribable) TimeNeedsSync characteristic!");
    peripheral.disconnect();
    return;
  } else if (!timeNeedsSyncChar.subscribe()) {
    Serial.println("Failed to subscribe to Peripheral's TimeNeedsSync characteristic!");
    peripheral.disconnect();
    return;
  }

  // retrieve the Reference-Timestamp characteristic
  BLECharacteristic referenceTimestampChar = peripheral.characteristic(UUID_CHARACTERISTIC_REFERENCE_TIMESTAMP);

  if (!referenceTimestampChar) {
    Serial.println("Peripheral does not have Reference-Timestamp characteristic!");
    peripheral.disconnect();
    return;
  } else if (!referenceTimestampChar.canWrite()) {
    Serial.println("Peripheral does not have a writable Reference-Timestamp characteristic!");
    peripheral.disconnect();
    return;
  }

  performSync(timeNeedsSyncChar, referenceTimestampChar);
  isSyncNeeded = false;

  event = 0;
  isEveryEventSent = false;
  eventsSentCount = 0;
  eventsReceivedCount = 0;

  hasReceivedSignal = false;
  startSysTime = now();

  while (peripheral.connected()) {
    // Input is being observed in the background by interrupt...
    // pollInput();

    handleInputSignalIfNeeded();

    unsigned long _now = now();
    if (!isEveryEventSent && _now - startSysTime >= events[event]) {
      // Scheduled event-time reached.

      // printTimestamp();

      Serial.print(_now - startSysTime);
      Serial.print(" -> ");
      
      Serial.print("EVENT occurred (eventTime=");
      Serial.print(events[event]);
      Serial.println("): Will notify Signalboy via BLE");

      if (isSyncNeeded) {
        Serial.print("FATAL: On Scheduled event-time reached: ");
        Serial.print("Unable to send Target-Timestamp, due to unsatisfied requirement");
        Serial.println(" (training required).");

        while (true);
      }

      // Send "Target-Timestamp" via BLE (event-timestamp + fixed-delay).
      sendTargetTimestamp(startSysTime + events[event] + DELAY_NORMALIZATION, targetTimestampChar);
      // sendTriggerTimer((byte) DELAY_NORMALIZATION, triggerTimerChar);

      eventsSentCount++;
      scheduleNextEvent(event);
    }

    updateIsSyncNeeded(timeNeedsSyncChar);
    if (
      isSyncNeeded
      // delay
      && now() - lastSyncNeededTime >= 10
    ) {
      performSync(timeNeedsSyncChar, referenceTimestampChar);
      isSyncNeeded = false;
    }
  }

  Serial.println("Peripheral disconnected");
}

unsigned long now() {
  while (millis() - lastMillis > 0) {
    sysTime++;
    lastMillis++;
  }

  return sysTime;
}

// Interrupt Service Routine (ISR)
void pollInput() {
  int inputState = digitalRead(PIN_INPUT);

  if (inputState != lastInputState) {
    if (inputState == HIGH) {
      lastRisingEdgeTime = now();
      hasReceivedSignal = true;
    }
  }

  lastInputState = inputState;
}

void handleInputSignalIfNeeded() {
  // Check whether we've received a signal via ISR (Interrupt Service Routine).
    if (hasReceivedSignal) {
      unsigned long lastRisingEdgeTimeCopy;

      eventsReceivedCount++;

      noInterrupts();

      lastRisingEdgeTimeCopy = lastRisingEdgeTime;        // capture value (volatile variable)
      hasReceivedSignal = false;

      interrupts();

      Serial.print(lastRisingEdgeTimeCopy - startSysTime);
      Serial.print(" -> ");

      Serial.print("HIGH detected");
      Serial.print(" (time passed since last signal: ");
      Serial.print(lastRisingEdgeTimeCopy - lastLastRisingEdgeTime);
      Serial.println(")");

      if (isEveryEventSent) {
        Serial.println("Did receive signal for last scheduled event.");
        Serial.println();
        Serial.println("SUMMARY:");
        Serial.print("Events count (sent/received):\t");
        Serial.println(String(eventsSentCount) + "/" + String(eventsReceivedCount));
      }

      lastLastRisingEdgeTime = lastRisingEdgeTimeCopy;
    }
}

void scheduleNextEvent(int &event) {
  size_t count = sizeof(events) / sizeof(events[0]);

  if (event + 1 < count) {
    event++;
  } else {
    Serial.println("Did sent all events.");
    isEveryEventSent = true;
  }
}

bool isSignalboyDevice(BLEDevice peripheral) {
  bool hasOutputService = false;

  for (int i = 0; i < peripheral.advertisedServiceUuidCount(); i++) {
    if (peripheral.advertisedServiceUuid(i) == UUID_SERVICE_OUTPUT) {
      hasOutputService = true;
      break;
    }
  }

  return hasOutputService;
}

void updateIsSyncNeeded(BLECharacteristic timeNeedsSyncChar) {
  // check whether the value of the subscribed characteristic has been updated
  if (timeNeedsSyncChar.valueUpdated()) {
    byte timeNeedsSyncValue = 0;
    timeNeedsSyncChar.readValue(timeNeedsSyncValue);

    if (timeNeedsSyncValue != 0) {
      lastSyncNeededTime = now();
      isSyncNeeded = true;
    }
  }
}

void performSync(BLECharacteristic timeNeedsSyncChar, BLECharacteristic referenceTimestampChar) {
  bool isSyncNeeded = true;
  for (int i = 0; i < 3; i++) {
    // Perform Sync/Training
    sendTrainingMsgs(referenceTimestampChar, TRAINING_MSGS_COUNT);

    // Ensure sync was successful
    unsigned long _millis = millis();
    while (
      !timeNeedsSyncChar.valueUpdated()
      && millis() - _millis < 100  // <- Timeout after 100ms
    )
      ;

    byte timeNeedsSyncValue = 0;
    timeNeedsSyncChar.readValue(timeNeedsSyncValue);

    if (timeNeedsSyncValue == 0) {
      isSyncNeeded = false;
      break;
    }
  }

  if (isSyncNeeded) {
    Serial.println("FATAL: Sync failed (Signalboy still reports timeNeedsSync).");
    while (true)
      ;
  } else {
    Serial.println("Sync succeeded.");
  }
}

// synced method
void sendTargetTimestamp(unsigned long value, BLECharacteristic targetTimestampChar) {
  targetTimestampChar.writeValue((uint32_t)value, false);
}

// unsynced method
void sendTriggerTimer(byte delay, BLECharacteristic triggerTimerChar) {
  triggerTimerChar.writeValue(delay, false);
}

void sendTrainingMsgs(BLECharacteristic referenceTimestampChar, int count) {
  printTimestamp();
  Serial.println("sendTrainingMsgs(): Will perform training...");

  unsigned long _now = now();

  unsigned long scheduledFiretimes[count];
  for (int i = 0; i < count; i++) {
    scheduledFiretimes[i] = _now + TRAINING_INTERVAL_IN_MILLIS * (i + 1);
  }

  for (int i = 0; i < count; i++) {
    sendTrainingMsg(referenceTimestampChar, scheduledFiretimes[i]);
  }

  // FIXME: Remove DEBUG code
  _now = now();  
  while (now() - _now < 100) {
    handleInputSignalIfNeeded();    
  }

  printTimestamp();
  Serial.println("sendTrainingMsgs(): Finished training.");
}

void sendTrainingMsg(BLECharacteristic referenceTimestampChar, unsigned long firetime) {
  // Actively sleep until message is to be send.
  while (now() - firetime > 0) {
    // no-op

    handleInputSignalIfNeeded();  // FIXME: Remove DEBUG code
    // Remove DEBUG code - END
  }

  referenceTimestampChar.writeValue(firetime, false);
  
  // FIXME: Remove DEBUG code
  printTimestamp();
  Serial.println("DEBUG: Sent Training Message (firetime=" + String(firetime) + ").");
  // Remove DEBUG code - END
}

void printDevice(BLEDevice peripheral) {
  // print address
  Serial.print("Address: ");
  Serial.println(peripheral.address());

  // print the local name, if present
  if (peripheral.hasLocalName()) {
    Serial.print("Local Name: ");
    Serial.println(peripheral.localName());
  }

  // print the advertised service UUIDs, if present
  if (peripheral.hasAdvertisedServiceUuid()) {
    Serial.print("Service UUIDs: ");
    for (int i = 0; i < peripheral.advertisedServiceUuidCount(); i++) {
      Serial.print(peripheral.advertisedServiceUuid(i));
      Serial.print(" ");
    }
    Serial.println();
  }

  // print the RSSI
  Serial.print("RSSI: ");
  Serial.println(peripheral.rssi());
}

String prompt(String prompt) {
  Serial.println(prompt);
  while (Serial.available() == 0) {}           //wait for data available
  String line = Serial.readStringUntil('\n');  //read until new-line or timeout
  line.trim();                                 // remove any \r \n whitespace at the end of the String
  return line;
}

void printTimestamp() {
  Serial.print(now());
  Serial.print(" -> ");
}
