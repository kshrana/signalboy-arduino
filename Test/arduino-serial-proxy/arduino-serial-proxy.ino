/*
  This sketch:
    - Receives events per Serial ('\n' ^= event) and signals respective inceptions
      via wire (HIGH on output-pin for short interval).

  The circuit:
    - Signalboy client (signaling events via Serial: '\n' ^= event)
    - MCU (`arduino-serial-proxy`)
      - USB: Connected to Signalboy client (Serial)
      - Output-pin 3: Wired to MCU running `arduino-monitor`
    - MCU (`arduino-monitor`)
      - Input-pin 2: Wired to Signalboy output (typically via 3.5mm jack)
      - Input-pin 9: Wired to MCU running `arduino-serial-proxy`
*/

#define PIN_OUTPUT 3
#define SIGNAL_HIGH_INTERVAL 20UL

unsigned long lastEventTime = 0;

void setup() {
  pinMode(PIN_OUTPUT, OUTPUT);

  Serial.begin(57600);
}

void loop() {
  // if there's any serial available, read it:
  while (Serial.available() > 0) {
    if (Serial.read() == '\n') {
      lastEventTime = millis();
    }

    updateOutput();
  }

  updateOutput();
}

void updateOutput() {
  digitalWrite(PIN_OUTPUT, (millis() - lastEventTime <= SIGNAL_HIGH_INTERVAL) ? HIGH : LOW);
}
