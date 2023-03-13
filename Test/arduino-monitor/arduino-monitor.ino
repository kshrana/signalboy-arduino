/*
  This sketch:
    - Receives events from a Signalboy device and logs out their inception to Serial
    - Optionally: Receives immediate notice of the occurrence of an event via wire
    - Test time is started after the first received event (via Signalboy signal or via 
      advance notice provided by MCU running `arduino-serial-proxy`)

  The circuit:
    - Signalboy client (optionally: signaling events via Serial: '\n' ^= event)
    - Optionally: MCU (`arduino-serial-proxy`)
      - USB: Connected to Signalboy client (Serial)
      - Output-pin 3: Wired to MCU running `arduino-monitor`
    - MCU (`arduino-monitor`)
      - Input-pin 2: Wired to Signalboy output (typically via 3.5mm jack)
      - Input-pin 9: Wired to MCU running `arduino-serial-proxy` (optionally)
*/

#define PIN_INPUT 2
#define PIN_INPUT_TEST 9

// Synced time measurement starts, after first
// (sync-)signal was received.
bool hasReceivedInitialSync = false;

unsigned long sysTime = 0;
unsigned long lastMillis = 0;

unsigned long startSysTime = 0;
bool isStartSysTimeSet = false;

/// `true`, after signal has been received on the input-pin (by interrupt).
volatile bool hasReceivedSignal = false;
/// The last time the input-pin was set to HIGH.
volatile unsigned long lastRisingEdgeTime = 0;
volatile int lastInputState = 0;

/// `true`, after signal has been received on the input-pin (by interrupt).
volatile bool hasReceivedTestSignal = false;
/// The last time the input-pin was set to HIGH.
volatile unsigned long lastTestRisingEdgeTime = 0;
volatile int lastTestInputState = 0;

void setup() {
  pinMode(PIN_INPUT, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_INPUT), pollInput, CHANGE);

  pinMode(PIN_INPUT_TEST, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_INPUT_TEST), pollTestInput, CHANGE);

  Serial.begin(9600);
  while (!Serial);
  delay(1000);

  printTimestamp();
  Serial.println("Entering \"monitor\"-mode (enter any input to cancel).");

  hasReceivedSignal = false;
  hasReceivedTestSignal = false;
}

void loop() {
  // Input is being observed in the background by interrupt...
  // pollInput();

  // Check whether we've received a signal via ISR (Interrupt Service Routine).
  if (hasReceivedSignal) {
    unsigned long lastRisingEdgeTimeCopy;

    noInterrupts();

    lastRisingEdgeTimeCopy = lastRisingEdgeTime;  // capture value (volatile variable)
    hasReceivedSignal = false;

    interrupts();

    if (!isStartSysTimeSet) {
      startSysTime = lastRisingEdgeTimeCopy;
      isStartSysTimeSet = true;
    }

    Serial.print(lastRisingEdgeTimeCopy - startSysTime);
    Serial.print(" -> ");
    Serial.println("HIGH detected");
  }

  // Check whether we've received a signal via ISR (Interrupt Service Routine).
  if (hasReceivedTestSignal) {
    unsigned long lastTestRisingEdgeTimeCopy;

    noInterrupts();

    lastTestRisingEdgeTimeCopy = lastTestRisingEdgeTime;  // capture value (volatile variable)
    hasReceivedTestSignal = false;

    interrupts();

    if (!isStartSysTimeSet) {
      startSysTime = lastTestRisingEdgeTimeCopy;
      isStartSysTimeSet = true;
    }

    Serial.print(lastTestRisingEdgeTimeCopy - startSysTime);
    Serial.print(" -> ");
    Serial.print("TEST -> ");
    Serial.println("HIGH detected");
  }

  if (Serial.available() > 0) {
    // Exit on any received input.
    Serial.println("Any input received -> bye");
    while (true);
  }
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

// Interrupt Service Routine (ISR)
void pollTestInput() {
  int inputState = digitalRead(PIN_INPUT_TEST);

  if (inputState != lastTestInputState) {
    if (inputState == HIGH) {
      lastTestRisingEdgeTime = now();
      hasReceivedTestSignal = true;
    }
  }

  lastTestInputState = inputState;
}

void printTimestamp() {
  Serial.print(millis());
  Serial.print(" -> ");
}

String readLine() {
  while (Serial.available() == 0) {}     //wait for data available
  String line = Serial.readStringUntil('\n');  //read until new-line or timeout
  line.trim();                        // remove any \r \n whitespace at the end of the String
  return line;
}
