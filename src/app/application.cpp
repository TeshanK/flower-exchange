#include "app/application.h"

#include <iostream>
#include <sstream>
#include <string>

#include "app/file_processing_pipeline.h"

Application::Application()
    : owned_file_processor_(std::make_unique<FileProcessingPipeline>()),
      input_(std::cin), file_processor_(*owned_file_processor_) {}

Application::Application(std::istream &input, FileProcessor &file_processor)
    : input_(input), file_processor_(file_processor) {}

Application::~Application() = default;

void Application::run() {
  std::string user_input;
  while (true) {
    std::getline(input_, user_input);
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
