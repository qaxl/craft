#include "app.hpp"
#include "util/error.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <new>

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
