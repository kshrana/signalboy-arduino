#pragma once

// Domains
#define ERROR_DOMAIN_DEFAULT 0

// Error Codes (and respective messages)
#define ERROR_CODE_BLE_INIT_FAILURE 0
#define ERROR_MSG_BLE_INIT_FAILURE "BLE-init fail."

namespace Signalboy {
class Error {
public:
  Error(byte domain, byte code, arduino::String msg);
  Error(byte code, arduino::String msg);
  virtual ~Error(){};

  virtual byte getDomain();
  virtual byte getCode();
  virtual arduino::String getMsg();

private:
  byte m_domain;
  byte m_code;
  arduino::String m_msg;
};
}