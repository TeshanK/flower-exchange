#pragma once

#include <string>

class FileProcessor {
public:
  virtual ~FileProcessor() = default;

  virtual void process_file(const std::string &input_path) = 0;
};
