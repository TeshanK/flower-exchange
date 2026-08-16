#include "app/application.h"

#include <iostream>
#include <sstream>
#include <string>

Application::Application() = default;

void Application::run() {
  std::string user_input;
  while (true) {
    std::getline(std::cin, user_input);
    if (user_input == "QUIT") {
      break;
    }

    std::stringstream input(user_input);
    std::string command;
    std::string path;
    input >> command >> path;

    if (command == "PROCESS" && !path.empty()) {
      file_processor_.process_file(path);
    }
  }
}
