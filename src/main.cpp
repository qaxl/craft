#include "app.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <new>

#include "util/error.hpp"

#ifndef I_DONT_CARE_IF_SOMETHING_GOES_WRONG_JUST_LET_ME
static_assert(sizeof(void *) == 8, "32-bit architecture IS NOT supported. You can still try building this project, but "
                                   "no support will be offered in case of something goes wrong.");
#endif

int main(int argc, char **argv) {
  // This looks absolutely terrifying, but gladly I won't have to touch it
  // again.
  try {
    craft::App app;

    if (!app.Run()) {
      std::cout << "Application quit because of an error: " << *craft::RuntimeError::GetErrorString() << std::endl;
      return EXIT_FAILURE;
    }
  } catch (const std::bad_alloc &ex) {
    std::cout << "Application ran out of memory: " << ex.what() << std::endl;
    return EXIT_FAILURE;
  } catch (const std::runtime_error &ex) {
    std::cout << "Application quit because of an runtime error: " << ex.what() << std::endl;
    return EXIT_FAILURE;
  } catch (const std::exception &ex) {
    std::cout << "Application quit because of an exception: " << ex.what() << std::endl;
    return EXIT_FAILURE;
  } catch (...) {
    std::cout << "Application quit because of an unknown exception." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
