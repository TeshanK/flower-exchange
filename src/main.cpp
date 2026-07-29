#include <memory>
#include <print>

#include "app/application.h"

int main() {
  std::println("Starting the application...");
  auto app = std::make_unique<Application>();
  app->run();
  return 0;
}
