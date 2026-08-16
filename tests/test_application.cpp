#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

#include "app/application.h"

namespace {

class RecordingFileProcessor final : public FileProcessor {
public:
  void process_file(const std::string &input_path) override {
    paths.push_back(input_path);
  }

  std::vector<std::string> paths;
};

} // namespace

TEST(ApplicationTest, DelegatesProcessCommandsUntilQuit) {
  std::istringstream input(
      "UNKNOWN ignored.csv\n"
      "PROCESS first.csv\n"
      "PROCESS\n"
      "PROCESS second.csv extra-argument\n"
      "QUIT\n"
      "PROCESS after-quit.csv\n");
  RecordingFileProcessor processor;
  Application application(input, processor);

  application.run();

  ASSERT_EQ(processor.paths.size(), 2u);
  EXPECT_EQ(processor.paths[0], "first.csv");
  EXPECT_EQ(processor.paths[1], "second.csv");
}
