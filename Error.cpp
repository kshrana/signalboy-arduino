#include <Arduino.h>
#include "Error.h"

using namespace Signalboy;

Error::Error(byte domain, byte code, arduino::String msg)
  : m_domain(domain),
    m_code(code),
    m_msg(msg) {}

Error::Error(byte code, arduino::String msg)
  : Error(ERROR_DOMAIN_DEFAULT, code, msg) {}

byte Error::getDomain() {
  return m_domain;
}

byte Error::getCode() {
  return m_code;
}

arduino::String Error::getMsg() {
  return m_msg;
}