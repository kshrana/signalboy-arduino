# Signalboy Reference Implementation – Peripheral (Arduino)
## Dependencies
* ArduinoBLE (I used v1.3.1 for initial development)

## Install on Arduino 33 IoT
First make sure to install ArduinoBLE:
> To use this library, open the Library Manager in the Arduino IDE and install it from there.

([ArduinoBLE - Arduino Reference](https://www.arduino.cc/reference/en/libraries/arduinoble/)
)

<br/>

Then compile & install using `arduino-cli`:
```bash
arduino-cli compile
arduino-cli upload -p <port>  # <port> might look like `/dev/cu.usbmodem1432101`. Find <port> by running `arduino-cli board list`.
```

## Usage
Program starts automatically on Arduino after startup and **waits for Serial Monitor before entering main-loop** (which scans for the [`node`-based Central](../node-peripheral/README.md) to connect to…)  
Thus you'll need to connect any serial-monitor that supports reading:
* E.g. (using `arduino-cli`):
  ```bash
  arduino-cli monitor -p <port>  # <port> might look like `/dev/cu.usbmodem1432101`. Find <port> by running `arduino-cli board list`.
  ```
* or (using `cat`):
  ```bash
  cat <port> # <port> might look like `/dev/cu.usbmodem1432101`. Find <port> by running `arduino-cli board list`.
  ```
