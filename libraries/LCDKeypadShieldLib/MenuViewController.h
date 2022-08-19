#pragma once

#include <memory>
#include <vector>
#include "ViewController.h"

struct IMenuItem {
  virtual ~IMenuItem() {}

  virtual arduino::String getLabel() = 0;
  virtual void onSelection() = 0;
};

class MenuViewController : public ViewController {
public:
  /// Note: Ownership of the `unique_ptr`-elements in `menuItems` will be
  /// transferred to the receiver:
  /// As a result `menuItems` will consist of empty pointers.
  MenuViewController(
    std::vector<std::unique_ptr<IMenuItem>> &&menuItems,
    arduino::String closeMenuActionLabel,
    arduino::String executeActionDescription);

  const std::vector<std::unique_ptr<IMenuItem>> &getMenuItems();
  /// Note: Ownership of the `unique_ptr`-elements in `menuItems` will be
  /// transferred to the receiver:
  /// As a result `menuItems` will consist of empty pointers.
  void setMenuItems(std::vector<std::unique_ptr<IMenuItem>> &menuItems);

  /// Resets menu state (i.e. current index).
  void reset();

  // ViewController
  arduino::String getLine1();
  arduino::String getLine2();
  void update();

  // IFirstResponder
  void onButtonPushed(Button_t button);

private:
  std::vector<std::unique_ptr<IMenuItem>> m_menuItems;
  /// Index of currently selected menu-item.
  uint8_t m_idx;

  arduino::String m_closeMenuActionLabel;
  arduino::String m_executeActionDescription;

  void navigateUp();
  void navigateDown();
  void select();

  /// Returns Menu-item at specified index, or NULL
  /// if out-of-bounds.
  IMenuItem *getMenuItemAt(uint8_t idx);
};