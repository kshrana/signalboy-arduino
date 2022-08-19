#include <Arduino.h>
#include "ErrorViewController.h"
#include "Resources.h"
#include <memory>

ErrorViewController::ErrorViewController()
  : m_error(NULL) {}

void ErrorViewController::setError(Signalboy::Error *error) {
  // auto errorMoved = std::move(error);
  // m_error = &errorMoved;
  m_error = error;
}

arduino::String ErrorViewController::getLine1() {
  arduino::String string = Resources::shared->errorCodeLabelPrefix;
  if (m_error) {
    string += "0x";
    string += String(m_error->getDomain(), HEX);
    string += " ";
    string += String(m_error->getCode(), HEX);
  }
  return string;
}

arduino::String ErrorViewController::getLine2() {
  arduino::String string = "(";
  if (m_error) {
    string += m_error->getMsg();
  }
  string += ")";
  return string;
}

void ErrorViewController::update() {}

void ErrorViewController::onButtonPushed(Button_t button) {}