#include <Arduino.h>
#include <LCDKeypadShieldLib.h>
#include "Resources.h"
#include "constants.h"

// Note: Never deleted, because Singleton.
Resources* Resources::shared = new Resources();

Resources::Resources()
  : hwRevision(String(HARDWARE_REVISION)), swRevision(String(SOFTWARE_REVISION)) {
  char versionDisplay_c[20];
  snprintf(
    versionDisplay_c,
    sizeof(versionDisplay_c),
    "(HW:%s|SW:%s)",
    hwRevision.c_str(),
    swRevision.c_str());
  this->versionDisplay = String(versionDisplay_c);

  char browseMenuAction_c[15] = "xx Browse menu";
  browseMenuAction_c[0] = CHAR_ARROW_UP;
  browseMenuAction_c[1] = CHAR_ARROW_DOWN;
  this->browseMenuAction = String(browseMenuAction_c);

  this->closeMenuAction = "***Close menu***";
  this->executeAction = "SELECT to exec.";
  this->rejectConnection = ">Reject conn.";
  this->errorCodeLabelPrefix = "Error: ";
}

arduino::String Resources::getStateLabel_init() {
  return "Starting...";
}

arduino::String Resources::getStateLabel_awaitingConnection() {
  return "Await. conn...";
}

arduino::String Resources::getStateLabel_connected(bool isSynced) {
  arduino::String string = "Con'ed(synced=";
  string += String(isSynced);
  string += ")";
  return string;
}
