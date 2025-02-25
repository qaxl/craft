#include "io.hpp"

namespace craft::io {
char *ReadFromFileIntoMemory(std::string_view file_name) {
  std::ifstream file(file_name.data(), std::ios::binary | std::ios::ate);

  auto pos = file.tellg();
  file.seekg(std::ios::beg);

  char *data = reinterpret_cast<char *>(calloc(1, pos));
  file.read(data, pos);

  if (file) {
    file.close();
    return data;
  } else {
    free(data);
    return nullptr;
  }
}

void WriteMemoryToFile(std::string_view file_name, void *data, size_t size) {
  std::ofstream file(file_name.data(), std::ios::binary);
  file.write(static_cast<char *>(data), size);

  // TODO: way to check errors
}
} // namespace craft::io
