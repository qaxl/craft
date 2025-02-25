#include "player.hpp"

#include "platform/window.hpp"

#include <iostream>

namespace craft {
Player::Player() : m_camera{m_pos.IntoGLMVector()} {}

void Player::RegisterWindowCallbacks(std::shared_ptr<Window> window) {
  // FIXME: this is a hack
  static Player *t = this;

  window->OnKeyEvent([](KeyboardKey key, bool pressed) { t->ProcessKeyEvent(key, pressed); });
}

void Player::ProcessKeyEvent(KeyboardKey key, bool pressed) {
  Action action = m_controls.GetAction(key);
  std::cout << static_cast<uint32_t>(key) << " " << static_cast<uint32_t>(action) << std::endl;
  if (action < Action::PlayerMoveActionCount) {
    m_actions_committed[static_cast<size_t>(action)] = pressed ? true : false;
  }
}
void Player::ProcessMouseMovement(float dx, float dy) { m_camera.ProcessMouseMovement(dx, dy); }

void Player::ProcessFrame(float delta) {
  for (size_t i = 0; i < static_cast<size_t>(Action::PlayerMoveActionCount); ++i) {
    if (m_actions_committed[i]) {
      m_camera.ProcessAction(static_cast<Action>(i), delta);
    }
  }
}

void Player::IncreaseMovementSpeed(float by_what) { m_camera.IncreaseMovementSpeedBy(by_what); }
} // namespace craft
