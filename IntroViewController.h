#include "ViewController.h"

class IntroViewController : public ViewController {
public:
  /// Default: `false`.
  bool m_isShowingAwaitingSerialPortNotice;

  IntroViewController();

  // ViewController
  arduino::String getLine1();
  arduino::String getLine2();
  void update();

  // IResponder
  void onButtonPushed(Button_t button);
};