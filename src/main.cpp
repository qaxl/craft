#include "app.hpp"

#include <cstdlib>
#include <iostream>

int main(int argc, char **argv) {
  auto app = craft::App::Init();
  if (!app) {
    std::cout << "Couldn't initialize the program, due to an error: "
              << craft::App::GetErrorString() << std::endl;
    return EXIT_FAILURE;
  } else {
    // All errors happening from this point on will be fatal, if they are not
    // handled gracefully. (The program will quit on an error.)
    app->Run();
  }
}
