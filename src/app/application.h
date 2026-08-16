#pragma once

#include <istream>
#include <memory>

#include "app/file_processor.h"

class Application {
public:
  Application();
  Application(std::istream &input, FileProcessor &file_processor);
  ~Application();

  Application(const Application &) = delete;
  Application &operator=(const Application &) = delete;

  // Runs interactive command loop (PROCESS/QUIT).
  void run();

private:
  std::unique_ptr<FileProcessor> owned_file_processor_;
  std::istream &input_;
  FileProcessor &file_processor_;
};
