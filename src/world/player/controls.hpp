#pragma once

#include <unordered_map>

#include "platform/keyboard.hpp"
#include "util/io.hpp"

namespace craft {
enum class Action : uint32_t {
  NoAction,

  PlayerMoveForward,
  PlayerMoveBackward,
  PlayerMoveLeft,
  PlayerMoveRight,
  PlayerMoveUp,
  PlayerMoveDown,
  PlayerMoveActionCount,

  Count = PlayerMoveActionCount,
};

struct ControlKeyData {
  KeyboardKey key;
  Action action;
};

class Controls {
public:
  Controls() {
    char *data = io::ReadFromFileIntoMemory("controls.cft");
    if (!data) {
      InitializeWithDefaultKeybindings();
      return;
    }

    uint16_t keybindings;
    memcpy(&keybindings, data, sizeof(keybindings));

    for (uint16_t i = 0; i < keybindings; ++i) {
      ControlKeyData key_data;
      memcpy(&key_data, data + 2 + i * sizeof(key_data), sizeof(key_data));

      m_keybindings[key_data.key] = key_data.action;
    }

    free(data);
  }

  void InitializeWithDefaultKeybindings() {
    m_keybindings.clear();

    m_keybindings[KeyboardKey::W] = Action::PlayerMoveForward;
    m_keybindings[KeyboardKey::S] = Action::PlayerMoveBackward;
    m_keybindings[KeyboardKey::A] = Action::PlayerMoveLeft;
    m_keybindings[KeyboardKey::D] = Action::PlayerMoveRight;

    m_keybindings[KeyboardKey::Space] = Action::PlayerMoveUp;
    m_keybindings[KeyboardKey::LShift] = Action::PlayerMoveDown;

    SaveKeybindings();
  }

  void SaveKeybindings() {
    size_t size = sizeof(uint16_t) + m_keybindings.size() * sizeof(ControlKeyData);
    char *data = static_cast<char *>(calloc(1, size));
    uint16_t count = m_keybindings.size();

    memcpy(data, &count, sizeof(count));

    size_t i = 0;
    for (auto keybind : m_keybindings) {
      ControlKeyData key_data{.key = keybind.first, .action = keybind.second};
      memcpy(data + 2 + i * sizeof(key_data), &key_data, sizeof(key_data));
      i += 1;
    }

    io::WriteMemoryToFile("controls.cft", data, size);
    free(data);
  }

  Action GetAction(KeyboardKey key) { return m_keybindings[key]; }

private:
  std::unordered_map<KeyboardKey, Action> m_keybindings;
};
} // namespace craft
