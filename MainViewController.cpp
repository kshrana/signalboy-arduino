#include <cstddef>
#include <Arduino.h>
#include "LCDKeypadShieldLib.h"
#include "MainViewController.h"
#include "MenuViewController.h"
#include "Resources.h"

// Constants
#define TIMEOUT_MENU 15 * 1000UL  // in ms

MainViewController::MainViewController()
  : m_menuViewControllerPtr(),  // Initialize with nullptr.
    m_text(""),
    m_isPresentingMenu(false),
    m_lastMenuInteractionTime(0) {}

void MainViewController::setText(arduino::String text) {
  m_text = text;
}

void MainViewController::setMenuItems(std::vector<std::unique_ptr<IMenuItem>> &menuItems) {
  // Add our CloseMenuMenuItem if menu is not empty (count > 0).
  if (menuItems.size() > 0) {
    menuItems.push_back(makeCloseMenuMenuItem());
  }
  getMenuViewController().setMenuItems(menuItems);
}

void MainViewController::presentMenu() {
  getMenuViewController().reset();

  // Only display menu if it is not empty.
  if (hasAnyMenuItems()) {
    m_isPresentingMenu = true;
    m_lastMenuInteractionTime = millis();
  } else {
    dismissMenu();
  }
}

void MainViewController::dismissMenu() {
  m_isPresentingMenu = false;
}

arduino::String MainViewController::getLine1() {
  if (m_isPresentingMenu) {
    return getMenuViewController().getLine1();
  } else {
    return m_text;
  }
}

arduino::String MainViewController::getLine2() {
  if (m_isPresentingMenu) {
    return getMenuViewController().getLine2();
  } else if (hasAnyMenuItems()) {
    return Resources::shared->browseMenuAction;
  } else {
    return "";
  }
}

void MainViewController::update() {
  if (m_isPresentingMenu && millis() - m_lastMenuInteractionTime >= TIMEOUT_MENU) {
    dismissMenu();
  }
}

// IFirstResponder
void MainViewController::onButtonPushed(Button_t button) {
  if (m_isPresentingMenu) {
    getMenuViewController().onButtonPushed(button);
    m_lastMenuInteractionTime = millis();
  } else {
    if (button == btnUP || button == btnDOWN) {
      presentMenu();
    }
  }
}

MainViewController::CloseMenuMenuItem::CloseMenuMenuItem(MainViewController *mainViewController)
  : m_mainViewController(mainViewController) {}

arduino::String MainViewController::CloseMenuMenuItem::getLabel() {
  return Resources::shared->closeMenuAction;
}

void MainViewController::CloseMenuMenuItem::onSelection() {
  m_mainViewController->dismissMenu();
}

MenuViewController &MainViewController::getMenuViewController() {
  if (!m_menuViewControllerPtr.get()) {
    m_menuViewControllerPtr.reset(std::move(makeMenuViewController()));
  }
  return *m_menuViewControllerPtr;
}

bool MainViewController::hasAnyMenuItems() {
  return getMenuViewController().getMenuItems().size() > 0;
}

// Factory

MenuViewController *MainViewController::makeMenuViewController() {
  return new MenuViewController({}, Resources::shared->closeMenuAction, Resources::shared->executeAction);
}

std::unique_ptr<MainViewController::CloseMenuMenuItem> MainViewController::makeCloseMenuMenuItem() {
  std::unique_ptr<CloseMenuMenuItem> menuItemPtr(new CloseMenuMenuItem{ this });
  return menuItemPtr;
}