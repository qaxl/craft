#pragma once

#include <cstdint>
#include <fstream>
#include <limits>
#include <string_view>
#include <unordered_map>

#include "error.hpp"

namespace craft {
struct ConfigHeader {
  uint16_t values;
};

enum class ConfigValueType : uint8_t {
  Integer,
};

struct ConfigValue {
  ConfigValueType type;
  char name[111];

  union {
    int64_t integer_signed;
    uint64_t integer_unsigned;
  };
};

class ConfigVariableStorage {
public:
  static ConfigValue *GetValue(std::string_view name) {
    static bool init = false;
    if (!init) {
      init = true;
      Init();
    }

    if (auto it = m_values.find(name); it != m_values.end()) {
      return &it->second;
    }
    return nullptr;
  }

  static void SetValue(std::string_view name, const ConfigValue &value) { m_values[name] = value; }

  static void Init() {
    std::ifstream config_file("config.cft");
    if (!config_file.is_open()) {
      return;
    }

    ConfigHeader header;
    config_file.read(reinterpret_cast<char *>(&header), sizeof(header));

    for (uint16_t value = 0; value < header.values; ++value) {
      if (!config_file) {
        break;
      }

      ConfigValue val;
      config_file.read(reinterpret_cast<char *>(&val), sizeof(val));

      m_values[val.name] = val;
    }
  }

  static void Flush() {
    std::ofstream config_file("config.cft");

    size_t data_size = sizeof(ConfigHeader) + m_values.size() * sizeof(ConfigValue);
    char *data = new char[data_size];
    if (m_values.size() > std::numeric_limits<uint16_t>::max()) {
      RuntimeError::Throw("too many config values in storage!");
    }

    ConfigHeader header{.values = static_cast<uint16_t>(m_values.size())};
    memcpy(data, &header, sizeof(header));

    size_t offset = sizeof(header);
    for (const auto &value : m_values) {
      memcpy(data + offset, &value, sizeof(value));
      offset += sizeof(value);
    }

    config_file.write(data, data_size);
    delete[] data;
  }

private:
  static inline std::unordered_map<std::string_view, ConfigValue> m_values;
};

template <typename T> class ConfigVariable {
public:
  ConfigVariable(std::string_view name, T default_value) {
    m_value = ConfigVariableStorage::GetValue(name);
    if (!m_value) {
      ConfigVariableStorage::SetValue(name, ConfigValue{.type = ConfigValueType::Integer});
    }
  }

  T Get() {
    if constexpr (std::is_integral_v<T>) {
      if constexpr (std::is_unsigned_v<T>) {
        return m_value->integer_unsigned;
      } else {
        return m_value->integer_signed;
      }
    } else {
      static_assert(false, "Unsupported type for a ConfigVariable!");
    }
  }

  void Set(T value) {
    if constexpr (std::is_integral_v<T>) {
      if constexpr (std::is_unsigned_v<T>) {
        m_value->integer_unsigned = value;
      } else {
        m_value->integer_signed = value;
      }
    } else {
      static_assert(false, "Unsupported type for a ConfigVariable");
    }

    // TODO: a queue to save values?
    ConfigVariableStorage::Flush();
  }

  T operator*() { return Get(); }

private:
  ConfigValue *m_value = nullptr;
};
} // namespace craft
