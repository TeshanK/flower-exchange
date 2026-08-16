#pragma once

#include "app/file_processing_pipeline.h"

class Application {
public:
  Application();
  ~Application() = default;

  // Runs interactive command loop (PROCESS/QUIT).
  void run();

private:
  FileProcessingPipeline file_processor_;
};
