#include "error.hpp"

#include <SDL3/SDL.h>

namespace craft {
void RuntimeError::SetErrorString(std::string_view description,
                                  ErrorFlags flags) {
  std::lock_guard<std::mutex> lock(s_mutex);

  auto trunc = description.substr(0, s_description.size());
  std::copy(trunc.begin(), trunc.end(), s_description.begin());

  // FIXME: this may induce a heap allocation, if the SDL error string is too
  // long? So, for now if an memory allocation issue happens, std::string will
  // throw an std::bad_alloc - and it is handled.
  if (flags & EF_AppendSDLErrors) {
    const char *error = SDL_GetError();
    size_t error_len = strlen(error);

    s_description.append(std::string_view(error, error_len));
  }

  if (flags & EF_AppendVkErrors) {
  }

  s_has_an_error.store(true);
}
} // namespace craft
