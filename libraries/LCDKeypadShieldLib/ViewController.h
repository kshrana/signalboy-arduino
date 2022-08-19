#pragma once

enum Button_t {
  btnRIGHT,
  btnUP,
  btnDOWN,
  btnLEFT,
  btnSELECT,

  // only used internally
  btnNONE,
};

class IResponder {
public:
  virtual ~IResponder(){};
  virtual void onButtonPushed(Button_t button) = 0;
};

class ViewController : public IResponder {
public:
  virtual ~ViewController(){};

  virtual arduino::String getLine1() = 0;
  virtual arduino::String getLine2() = 0;
  virtual void update() = 0;

  // IResponder
  virtual void onButtonPushed(Button_t button) = 0;
};