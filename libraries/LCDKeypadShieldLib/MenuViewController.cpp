#include <Arduino.h>
#include "MenuViewController.h"

MenuViewController::MenuViewController(
  std::vector<std::unique_ptr<IMenuItem>> &&menuItems,
  arduino::String closeMenuActionLabel,
  arduino::String executeActionDescription)
  : m_menuItems(std::move(menuItems)),
    m_idx(0),
    m_closeMenuActionLabel(closeMenuActionLabel),
    m_executeActionDescription(executeActionDescription) {}

const std::vector<std::unique_ptr<IMenuItem>> &MenuViewController::getMenuItems() {
  return m_menuItems;
}

void MenuViewController::setMenuItems(std::vector<std::unique_ptr<IMenuItem>> &menuItems) {
  m_menuItems.clear();
  for (uint8_t i = 0; i < menuItems.size(); i++) {
    m_menuItems.push_back(std::move(menuItems[i]));
  }
  m_menuItems.shrink_to_fit();
}

void MenuViewController::reset() {
  m_idx = 0;
}

// ViewController

String MenuViewController::getLine1() {
  IMenuItem *item = getMenuItemAt(m_idx);
  if (!item) {
    return m_closeMenuActionLabel;
  }

  return item->getLabel();
}

String MenuViewController::getLine2() {
  return m_executeActionDescription;
}

void MenuViewController::update() {}

// IFirstResponder

void MenuViewController::onButtonPushed(Button_t button) {
  switch (button) {
    case btnRIGHT:
      {
        select();
        break;
      }
    case btnLEFT:
      {
        break;
      }
    case btnUP:
      {
        navigateUp();
        break;
      }
    case btnDOWN:
      {
        navigateDown();
        break;
      }
    case btnSELECT:
      {
        select();
        break;
      }
    case btnNONE:
      {
        break;
      }
  }
}

void MenuViewController::navigateUp() {
  m_idx = min(m_idx - 1, m_menuItems.size() - 1);
}

void MenuViewController::navigateDown() {
  m_idx = (m_idx + 1) % m_menuItems.size();
}

void MenuViewController::select() {
  IMenuItem *menuItem = getMenuItemAt(m_idx);
  if (menuItem) {
    menuItem->onSelection();
  }
}

IMenuItem *MenuViewController::getMenuItemAt(uint8_t idx) {
  if (idx >= m_menuItems.size()) {
    return NULL;
  }

  return m_menuItems[idx].get();
}