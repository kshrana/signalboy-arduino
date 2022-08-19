#pragma once

#include <memory>
#include <vector>
#include "ViewController.h"
#include "MenuViewController.h"

class MainViewController : public ViewController {
public:
  MainViewController();

  void setText(arduino::String text);
  void setMenuItems(std::vector<std::unique_ptr<IMenuItem>> &menuItems);

  void presentMenu();
  void dismissMenu();

  // ViewController
  arduino::String getLine1();
  arduino::String getLine2();
  void update();

  // IFirstResponder
  void onButtonPushed(Button_t button);

private:
  struct CloseMenuMenuItem : public IMenuItem {
    CloseMenuMenuItem(MainViewController *mainViewController);

    arduino::String getLabel();
    void onSelection();

  private:
    MainViewController *m_mainViewController;
  };

  // Child View-Controller
  std::unique_ptr<MenuViewController> m_menuViewControllerPtr;

  arduino::String m_text;

  bool m_isPresentingMenu;
  /// Time when user last interacted with the menu.
  /// (Used for menu timeout.)
  unsigned long m_lastMenuInteractionTime;

  MenuViewController &getMenuViewController();

  /// Returns `true`, if any menu-items have been set,
  /// s. `setMenuItems()`.
  bool hasAnyMenuItems();

  // Factory
  MenuViewController *makeMenuViewController();
  std::unique_ptr<CloseMenuMenuItem> makeCloseMenuMenuItem();
};