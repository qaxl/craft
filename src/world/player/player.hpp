#pragma once

#include "camera.hpp"
#include "controls.hpp"
#include "game_mode.hpp"
#include "math/vec.hpp"

namespace craft {
class Window;

class Player {
public:
  Player();

  void RegisterWindowCallbacks(std::shared_ptr<Window> window);
  void ProcessKeyEvent(KeyboardKey key, bool pressed);
  void ProcessMouseMovement(float dx, float dy);
  void ProcessFrame(float delta);

  void IncreaseMovementSpeed(float by_what = 1.0f);

  // TODO: temporary to keep compability with current renderer
  Camera *GetCamera() { return &m_camera; }

private:
  Vec3f m_pos{0, 10, 0};
  Vec3f m_movement{0, 0, 0};

  float m_move_speed = 12.7f;
  bool m_actions_committed[static_cast<size_t>(Action::PlayerMoveActionCount)]{};

  Camera m_camera;
  Controls m_controls;
  GameMode m_mode = GameMode::Mortal;
};
} // namespace craft
