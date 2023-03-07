/*******************************************************

This program allows Signalboy to use the LCD panel and
the buttons on D1 Robot LCD Keypad Shield.

Based on program by Michael Jonathan, February 2019
Modified by Kishor Rana, August 2022
Find the original work on [GitHub](https://github.com/mich1342/D1RobotLCDKeypadShieldArduinoUno/blob/master/KeypadExample.ino).

********************************************************/

#pragma once

#include <memory>
#include "ViewController.h"

// lcd.write(CHAR_ARROW_UP); // prints "↑"-character (arrow-up)
extern const byte CHAR_ARROW_UP;
// lcd.write(CHAR_ARROW_DOWN); // prints "↓"-character (arrow-down)
extern const byte CHAR_ARROW_DOWN;

class LiquidCrystal;
class LCDKeypadScreen {
public:
  LCDKeypadScreen(
    /// Number of columns (characters in a line)
    /// supported by the LCD-display.
    uint8_t lcdColumnCount,
    uint8_t pin_rs,
    uint8_t pin_enable,
    uint8_t pin_d0,
    uint8_t pin_d1,
    uint8_t pin_d2,
    uint8_t pin_d3,
    uint8_t pin_backlight_enable);
  ~LCDKeypadScreen();

  ViewController *getRootViewController();
  void setRootViewController(ViewController *viewController);

  void setup();
  /// NOTE: This function expects to be called continously (i.e. your `loop()`-function).
  /// That's because the A0-pin is polled for any updates, that may indicate whether any of
  /// the keypad's button have been pushed/released.
  void update();

private:
  /// The number of characters in a line supported by
  /// the lcd-display.
  uint8_t m_lcdColumnCount;
  uint8_t m_pin_backlight_enable;

  /// The string currently displayed by LiquidCrystal.
  arduino::String m_lcdLine1;
  /// The string currently displayed by LiquidCrystal.
  arduino::String m_lcdLine2;
  /// The string that will be set on next update (`updateScreen()`).
  arduino::String m_nextLcdLine1;
  /// The string that will be set on next update (`updateScreen()`).
  arduino::String m_nextLcdLine2;

  ViewController *m_rootViewControllerPtr;

  int m_lastAdcKeyIn;
  unsigned long m_lastAnalogReadTime;
  Button_t readLcdButtons();

  /// `true`, if backlight is active.
  bool m_isBacklightActive;
  Button_t m_buttonState;            // the current reading from the A0 pin
  Button_t m_lastButtonState;        // the previous reading from the input pin
  unsigned long m_lastDebounceTime;  // the last time the output pin was toggled
  /// Timestamp created when user last interacted with
  /// the keypad.
  unsigned long m_lastUserInteractionTime;
  std::unique_ptr<LiquidCrystal> m_lcdPtr;
  void setBacklightActive(bool active);
  void handleBacklightTimeoutIfNeeded(unsigned long now);
  void lcdWriteLine(const char *string, size_t num, uint8_t line);
};