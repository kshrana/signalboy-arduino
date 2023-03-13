# arduino-test-client

Sketch that allows testing the functionality of a Signalboy device and evaluation of the latency accompanying its signaling feature.

## Overview (Test-setup)

- Clock: `millis()`
- Events: Hardcoded event-dates (s. [events.csv](events.csv))

#### Devices
- Arduino NANO 33 IoT (`arduino-test-client`)
- Signalboy (`signalboy-arduino`)

#### Method of transmission
- BLE (synced-method / reference-timestamp)

#### Diagram
Arduino NANO 33 IoT (`arduino-test-client`) <- BLE (synced-method) -> Signalboy (`signalboy-arduino`)

## Arduino NANO 33 IoT


## Signalboy
- Clock: millis()
