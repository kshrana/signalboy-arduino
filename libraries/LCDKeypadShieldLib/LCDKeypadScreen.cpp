#include <sys/_stdint.h>
/*******************************************************

This program allows Signalboy to use the LCD panel and
the buttons on D1 Robot LCD Keypad Shield.

Based on program by Michael Jonathan, February 2019
Modified by Kishor Rana, August 2022
Find the original work on [GitHub](https://github.com/mich1342/D1RobotLCDKeypadShieldArduinoUno/blob/master/KeypadExample.ino).

********************************************************/

#include <Arduino.h>
#include <LiquidCrystal.h>
#include "LCDKeypadScreen.h"
#include "LCDKeypadShieldLib.h"
#include "AnalogReadingModeSetup.h"

// Constants
#define DEBOUNCE_DELAY 50UL            // in ms
#define TIMEOUT_BACKLIGHT 20 * 1000UL  // in ms

const byte CHAR_ARROW_UP = 0x01;
const byte CHAR_ARROW_DOWN = 0x02;

LCDKeypadScreen::LCDKeypadScreen(
  uint8_t lcdColumnCount,
  uint8_t pin_rs,
  uint8_t pin_enable,
  uint8_t pin_d0,
  uint8_t pin_d1,
  uint8_t pin_d2,
  uint8_t pin_d3,
  uint8_t pin_backlight_enable)
  : m_lcdColumnCount(lcdColumnCount),
    m_lcdLine1(""),
    m_lcdLine2(""),
    m_nextLcdLine1(""),
    m_nextLcdLine2(""),
    m_isBacklightActive(false),
    m_buttonState(btnNONE),
    m_lastButtonState(btnNONE),
    m_lastDebounceTime(0),
    m_lastUserInteractionTime(0),
    // select the pins used on the LCD panel
    m_lcdPtr(new LiquidCrystal(pin_rs, pin_enable, pin_d0, pin_d1, pin_d2, pin_d3)),
    m_pin_backlight_enable(pin_backlight_enable) {}

LCDKeypadScreen::~LCDKeypadScreen() {}

ViewController *LCDKeypadScreen::getRootViewController() {
  return m_rootViewControllerPtr;
}

void LCDKeypadScreen::setRootViewController(ViewController *viewController) {
  m_rootViewControllerPtr = viewController;
}

Button_t read_LCD_buttons() {
  int adc_key_in = analogReadIfSafe(A0);  // read the value from the sensor
  // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
  // we add approx 50 to those values and check to see if we are close
  if (adc_key_in > 950) return btnNONE;  // We make this the 1st option for speed reasons since it will be the most likely result
  // For V1.0 use this threshold
  if (adc_key_in < 120) return btnRIGHT;
  if (adc_key_in < 260) return btnUP;
  if (adc_key_in < 500) return btnDOWN;
  if (adc_key_in < 700) return btnLEFT;
  if (adc_key_in < 950) return btnSELECT;

  return btnNONE;  // when all others fail, return this...
}

void LCDKeypadScreen::setBacklightActive(bool active) {
  m_isBacklightActive = active;
  digitalWrite(m_pin_backlight_enable, m_isBacklightActive ? HIGH : LOW);
}

void LCDKeypadScreen::handleBacklightTimeoutIfNeeded(unsigned long now) {
  if (m_isBacklightActive && (now - m_lastUserInteractionTime) >= TIMEOUT_BACKLIGHT) {
    setBacklightActive(false);
  }
}

void LCDKeypadScreen::setup() {
  setupAnalogReadingFor5vAREF();

  pinMode(m_pin_backlight_enable, OUTPUT);
  setBacklightActive(true);

  // Similar to: ↑
  byte arrowUp[8] = {
    B00000,
    B00100,
    B01110,
    B10101,
    B00100,
    B00100,
    B00100,
  };
  m_lcdPtr->createChar(CHAR_ARROW_UP, arrowUp);

  // Similar to: ↓
  byte arrowDown[8] = {
    B00000,
    B00100,
    B00100,
    B00100,
    B10101,
    B01110,
    B00100,
  };
  m_lcdPtr->createChar(CHAR_ARROW_DOWN, arrowDown);
  m_lcdPtr->begin(m_lcdColumnCount, 2);  // start the LiquidCrystal-library
}

void LCDKeypadScreen::lcdWriteLine(const char *string, size_t num, uint8_t line) {
  char data[num];
  strncpy(data, string, num);
  // We'll make sure, that the string is null-terminated
  // (String will be truncated, if it's too long).
  data[num - 1] = '\0';

  m_lcdPtr->setCursor(0, line);
  for (int i = 0; i < num; i++) {
    char character = data[i];

    if (character == '\0') {
      break;
    }
    m_lcdPtr->write(data[i]);
  }
}

void LCDKeypadScreen::update() {
  unsigned long now = millis();
  handleBacklightTimeoutIfNeeded(now);

  if (m_rootViewControllerPtr) {
    m_rootViewControllerPtr->update();
    
    m_nextLcdLine1 = m_rootViewControllerPtr->getLine1();
    m_nextLcdLine2 = m_rootViewControllerPtr->getLine2();
  } else {
    m_nextLcdLine1 = "";
    m_nextLcdLine2 = "";
  }

  // Only update display if necessary.
  bool isDisplayUpdateNeeded = m_nextLcdLine1 != m_lcdLine1 || m_nextLcdLine2 != m_lcdLine2;
  if (isDisplayUpdateNeeded) {
    m_lcdPtr->clear();

    // line1
    lcdWriteLine(m_nextLcdLine1.c_str(), m_lcdColumnCount + 1, 0);
    m_lcdLine1 = m_nextLcdLine1;

    // line2
    lcdWriteLine(m_nextLcdLine2.c_str(), m_lcdColumnCount + 1, 1);
    m_lcdLine2 = m_nextLcdLine2;
  }

  Button_t reading = read_LCD_buttons();
  // If the switch changed, due to noise or pressing:
  if (reading != m_lastButtonState) {
    // reset the debouncing timer
    m_lastDebounceTime = now;
  }

  if ((now - m_lastDebounceTime) > DEBOUNCE_DELAY) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != m_buttonState) {
      m_buttonState = reading;

      // Only handle updated state when user has pushed any button.
      if (m_buttonState != btnNONE) {
        m_lastUserInteractionTime = now;
        setBacklightActive(true);

        if (m_rootViewControllerPtr) {
          m_rootViewControllerPtr->onButtonPushed(m_buttonState);
        }
      }
    }
  }

  // Save the reading. Next time through the loop, it'll be the lastButtonState:
  m_lastButtonState = reading;
}