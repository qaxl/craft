#pragma once

#include <bitset>
#include <utility>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <volk.h>

#include "SDL3/SDL_scancode.h"
#include "util/error.hpp"

struct SDL_Window;

namespace craft {
// Generated at 05/01/25 (DD/MM/YY), update if necessary.
enum class KeyboardKey {
  A = SDL_SCANCODE_A,
  B = SDL_SCANCODE_B,
  C = SDL_SCANCODE_C,
  D = SDL_SCANCODE_D,
  E = SDL_SCANCODE_E,
  F = SDL_SCANCODE_F,
  G = SDL_SCANCODE_G,
  H = SDL_SCANCODE_H,
  I = SDL_SCANCODE_I,
  J = SDL_SCANCODE_J,
  K = SDL_SCANCODE_K,
  L = SDL_SCANCODE_L,
  M = SDL_SCANCODE_M,
  N = SDL_SCANCODE_N,
  O = SDL_SCANCODE_O,
  P = SDL_SCANCODE_P,
  Q = SDL_SCANCODE_Q,
  R = SDL_SCANCODE_R,
  S = SDL_SCANCODE_S,
  T = SDL_SCANCODE_T,
  U = SDL_SCANCODE_U,
  V = SDL_SCANCODE_V,
  W = SDL_SCANCODE_W,
  X = SDL_SCANCODE_X,
  Y = SDL_SCANCODE_Y,
  Z = SDL_SCANCODE_Z,

  Num1 = SDL_SCANCODE_1,
  Num2 = SDL_SCANCODE_2,
  Num3 = SDL_SCANCODE_3,
  Num4 = SDL_SCANCODE_4,
  Num5 = SDL_SCANCODE_5,
  Num6 = SDL_SCANCODE_6,
  Num7 = SDL_SCANCODE_7,
  Num8 = SDL_SCANCODE_8,
  Num9 = SDL_SCANCODE_9,
  Num0 = SDL_SCANCODE_0,

  Return = SDL_SCANCODE_RETURN,
  Escape = SDL_SCANCODE_ESCAPE,
  Backspace = SDL_SCANCODE_BACKSPACE,
  Tab = SDL_SCANCODE_TAB,
  Space = SDL_SCANCODE_SPACE,

  Minus = SDL_SCANCODE_MINUS,
  Equals = SDL_SCANCODE_EQUALS,
  LeftBracket = SDL_SCANCODE_LEFTBRACKET,
  RightBracket = SDL_SCANCODE_RIGHTBRACKET,
  Backslash = SDL_SCANCODE_BACKSLASH,
  Semicolon = SDL_SCANCODE_SEMICOLON,
  Apostrophe = SDL_SCANCODE_APOSTROPHE,
  Grave = SDL_SCANCODE_GRAVE,
  Comma = SDL_SCANCODE_COMMA,
  Period = SDL_SCANCODE_PERIOD,
  Slash = SDL_SCANCODE_SLASH,

  CapsLock = SDL_SCANCODE_CAPSLOCK,

  F1 = SDL_SCANCODE_F1,
  F2 = SDL_SCANCODE_F2,
  F3 = SDL_SCANCODE_F3,
  F4 = SDL_SCANCODE_F4,
  F5 = SDL_SCANCODE_F5,
  F6 = SDL_SCANCODE_F6,
  F7 = SDL_SCANCODE_F7,
  F8 = SDL_SCANCODE_F8,
  F9 = SDL_SCANCODE_F9,
  F10 = SDL_SCANCODE_F10,
  F11 = SDL_SCANCODE_F11,
  F12 = SDL_SCANCODE_F12,

  PrintScreen = SDL_SCANCODE_PRINTSCREEN,
  ScrollLock = SDL_SCANCODE_SCROLLLOCK,
  Pause = SDL_SCANCODE_PAUSE,
  Insert = SDL_SCANCODE_INSERT,
  Home = SDL_SCANCODE_HOME,
  PageUp = SDL_SCANCODE_PAGEUP,
  Delete = SDL_SCANCODE_DELETE,
  End = SDL_SCANCODE_END,
  PageDown = SDL_SCANCODE_PAGEDOWN,
  Right = SDL_SCANCODE_RIGHT,
  Left = SDL_SCANCODE_LEFT,
  Down = SDL_SCANCODE_DOWN,
  Up = SDL_SCANCODE_UP,

  NumLockClear = SDL_SCANCODE_NUMLOCKCLEAR,
  KP_Divide = SDL_SCANCODE_KP_DIVIDE,
  KP_Multiply = SDL_SCANCODE_KP_MULTIPLY,
  KP_Minus = SDL_SCANCODE_KP_MINUS,
  KP_Plus = SDL_SCANCODE_KP_PLUS,
  KP_Enter = SDL_SCANCODE_KP_ENTER,
  KP_1 = SDL_SCANCODE_KP_1,
  KP_2 = SDL_SCANCODE_KP_2,
  KP_3 = SDL_SCANCODE_KP_3,
  KP_4 = SDL_SCANCODE_KP_4,
  KP_5 = SDL_SCANCODE_KP_5,
  KP_6 = SDL_SCANCODE_KP_6,
  KP_7 = SDL_SCANCODE_KP_7,
  KP_8 = SDL_SCANCODE_KP_8,
  KP_9 = SDL_SCANCODE_KP_9,
  KP_0 = SDL_SCANCODE_KP_0,
  KP_Period = SDL_SCANCODE_KP_PERIOD,

  LCtrl = SDL_SCANCODE_LCTRL,
  LShift = SDL_SCANCODE_LSHIFT,
  LAlt = SDL_SCANCODE_LALT,
  LGUI = SDL_SCANCODE_LGUI,
  RCtrl = SDL_SCANCODE_RCTRL,
  RShift = SDL_SCANCODE_RSHIFT,
  RAlt = SDL_SCANCODE_RALT,
  RGUI = SDL_SCANCODE_RGUI,

  Mode = SDL_SCANCODE_MODE,

  Sleep = SDL_SCANCODE_SLEEP,
  Wake = SDL_SCANCODE_WAKE,

  MediaPlay = SDL_SCANCODE_MEDIA_PLAY,
  MediaPause = SDL_SCANCODE_MEDIA_PAUSE,
  MediaStop = SDL_SCANCODE_MEDIA_STOP,
  MediaNextTrack = SDL_SCANCODE_MEDIA_NEXT_TRACK,
  MediaPreviousTrack = SDL_SCANCODE_MEDIA_PREVIOUS_TRACK,

  SoftLeft = SDL_SCANCODE_SOFTLEFT,
  SoftRight = SDL_SCANCODE_SOFTRIGHT,
  Call = SDL_SCANCODE_CALL,
  EndCall = SDL_SCANCODE_ENDCALL,

  // Reserved = SDL_SCANCODE_RESERVED,
  Count = SDL_SCANCODE_COUNT
};

class Window {
public:
  Window(int width, int height, const char *title = "craft engine window");
  ~Window();

  Window(const Window &) = delete;
  Window(Window &&window) {
    m_window = window.m_window;
    m_window_is_open = window.m_window_is_open;

    window.m_window_is_open = false;
    window.m_window = nullptr;
  }

  void PollEvents();
  bool IsOpen() const { return m_window_is_open; }

  SDL_Window *GetHandle() { return m_window; }
  void GetSize(int &width, int &height) const { SDL_GetWindowSizeInPixels(m_window, &width, &height); }

  std::pair<uint32_t, uint32_t> GetSize() const {
    int width, height;
    GetSize(width, height);
    // this converts them to uint, "safely"
    return std::make_pair(width, height);
  }

  VkSurfaceKHR CreateSurface(VkInstance instance) {
    VkSurfaceKHR surface;
    if (!SDL_Vulkan_CreateSurface(m_window, instance, nullptr, &surface)) {
      RuntimeError::Throw("Couldn't create a Vulkan surface.");
    }
    return surface;
  }

  bool IsKeyPressed(KeyboardKey key) { return m_key_down.test(static_cast<size_t>(key)); }
  std::pair<float, float> GetRelativeMouseMotion() { return m_relative_motion; }
  float GetMouseScroll() { return m_mouse_scroll; }

private:
  SDL_Window *m_window = nullptr;
  bool m_window_is_open = true;

  std::bitset<SDL_SCANCODE_COUNT> m_key_down;
  std::pair<float, float> m_relative_motion = std::make_pair(0.0f, 0.0f);
  float m_mouse_scroll = 0.0f;
  // std::vector<bool> m_key_down2;
};
} // namespace craft
