#pragma once

#include <cstdlib>
#include <fstream>
#include <stdlib.h>
#include <string_view>

namespace craft::io {
char *ReadFromFileIntoMemory(std::string_view file_name);
void WriteMemoryToFile(std::string_view file_name, void *data, size_t size);
} // namespace craft::io
