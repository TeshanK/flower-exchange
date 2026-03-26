#include <memory>

#include "app/application.h"

int main() {
  auto app = std::make_unique<Application>();
  app->run();
  return 0;
}
