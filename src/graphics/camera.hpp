#pragma once

#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>

#include <algorithm>
#include <iostream>

inline std::ostream &operator<<(std::ostream &o, glm::vec3 &v) {
  o << "vec3{" << v.x << ':' << v.y << ':' << v.z << '}';
  return o;
}

namespace craft {
enum class CameraMovement { Forward, Backward, Left, Right, Up, Down };

constexpr float const kYaw = -90.0f;
constexpr float const kPitch = 0.0f;
constexpr float const kSpeed = 2.5f;
constexpr float const kSensitivity = 0.1f;
constexpr float const kFov = 45.0f;

class Camera {
public:
  Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = kYaw,
         float pitch = kPitch)
      : m_front{glm::vec3(0.0f, 0.0f, -1.0f)}, m_position{position}, m_world_up{up}, m_yaw{yaw}, m_pitch{pitch} {}

  glm::mat4 ViewMatrix() const { return glm::lookAtLH(m_position, m_position + m_front, m_up); }

  float GetFov() const { return m_fov; }
  glm::vec3 GetForward() const { return m_front; }
  glm::vec3 GetPosition() const { return m_position; }

  void ProcessKeyboard(CameraMovement direction, float delta) {
    float velocity = m_movement_speed * delta;
    glm::vec3 front = m_front;
    front.y = 0.0f;

    switch (direction) {
    case CameraMovement::Forward:
      m_position += front * velocity;
      break;

    case CameraMovement::Backward:
      m_position -= front * velocity;
      break;

    case CameraMovement::Left:
      m_position += m_right * velocity;
      break;

    case CameraMovement::Right:
      m_position -= m_right * velocity;
      break;

    case CameraMovement::Up:
      m_position -= m_up * velocity;
      break;

    case CameraMovement::Down:
      m_position += m_up * velocity;
      break;
    }
  }

  void ProcessMouseMovement(float x, float y, bool constrain = true) {
    x *= m_mouse_sensitivity;
    y *= m_mouse_sensitivity;

    m_yaw -= x;
    m_pitch += y;

    if (constrain) {
      m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
    }

    UpdateVectors();
  }

  void ProcessMouseScroll(float y) {
    m_fov -= y;
    m_fov = std::clamp(m_fov, 1.0f, 90.0f);
  }

  void IncreaseMovementSpeedBy(float what) { m_movement_speed += what; }

private:
  void UpdateVectors() {
    glm::vec3 front = glm::vec3(cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch)), sin(glm::radians(m_pitch)),
                                sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch)));
    m_front = glm::normalize(front);
    m_right = glm::normalize(glm::cross(m_front, m_world_up));
    m_up = glm::normalize(glm::cross(m_right, m_front));
  }

private:
  glm::vec3 m_position;
  glm::vec3 m_front;
  glm::vec3 m_up;
  glm::vec3 m_right;
  glm::vec3 m_world_up;

  float m_yaw = kYaw;
  float m_pitch = kPitch;

  float m_movement_speed = kSpeed;
  float m_mouse_sensitivity = kSensitivity;
  float m_fov = kFov;
};
} // namespace craft
