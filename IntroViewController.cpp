#include <Arduino.h>
#include "IntroViewController.h"
#include "Resources.h"

IntroViewController::IntroViewController()
  : m_isShowingAwaitingSerialPortNotice(false) {}

arduino::String IntroViewController::getLine1() {
  if (!m_isShowingAwaitingSerialPortNotice) {
    return "Signalboy";
  } else {
    return "Waiting (serial)";
  }
}

arduino::String IntroViewController::getLine2() {
  if (!m_isShowingAwaitingSerialPortNotice) {
    return Resources::shared->versionDisplay;
  } else {
    return "...";
  }
}

void IntroViewController::update() {}

void IntroViewController::onButtonPushed(Button_t button) {}