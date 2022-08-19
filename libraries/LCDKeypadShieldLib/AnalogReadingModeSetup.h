/**
  Arduino Nano 33 IoT's operating voltage is 3.3V,
  but the LCD Keypad Shield was designed for 5.
  By connecting AREF to the Nano 33 IoT's VUSB (5V)
  we'll make sure, that the original design works
  for reading the pushed button.
*/
void setupAnalogReadingFor5vAREF();

int analogReadIfSafe(pin_size_t pinNumber);
