#include "ViewController.h"
#include "Error.h"

class ErrorViewController : public ViewController {
public:
  ErrorViewController();

  void setError(Signalboy::Error *error);

  // ViewController
  arduino::String getLine1();
  arduino::String getLine2();
  void update();

  // IResponder
  void onButtonPushed(Button_t button);

private:
  Signalboy::Error *m_error;
};