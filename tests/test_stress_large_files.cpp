#include <gtest/gtest.h>
#include <sys/wait.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#ifndef FLOWER_EXCHANGE_BIN
#define FLOWER_EXCHANGE_BIN ""
#endif

#ifndef FLOWER_EXCHANGE_REPO_ROOT
#define FLOWER_EXCHANGE_REPO_ROOT ""
#endif

namespace {

std::filesystem::path repo_path(const std::string &rel) {
  return std::filesystem::path(FLOWER_EXCHANGE_REPO_ROOT) / rel;
}

std::filesystem::path
output_path_for_input(const std::filesystem::path &input_path) {
  const auto filename = input_path.stem().string() + "_reports.csv";

  auto cwd_output = std::filesystem::current_path() / "output" / filename;
  if (std::filesystem::exists(cwd_output)) {
    return cwd_output;
  }

  auto repo_output = repo_path("output") / filename;
  if (std::filesystem::exists(repo_output)) {
    return repo_output;
  }

  // Default to cwd for first-time run assertions under ctest.
  return cwd_output;
}

bool env_flag_enabled(const char *name) {
  const char *value = std::getenv(name);
  if (!value) {
    return false;
  }
  return std::string(value) == "1";
}

double env_double(const char *name, double fallback) {
  const char *value = std::getenv(name);
  if (!value) {
    return fallback;
  }
  return std::atof(value);
}

int run_process_once(const std::filesystem::path &input_path) {
  const auto cmd_file =
      std::filesystem::temp_directory_path() /
      ("flower_exchange_stress_cmds_" + input_path.stem().string() + ".txt");

  std::ofstream out(cmd_file);
  out << "PROCESS " << input_path.string() << '\n';
  out << "QUIT\n";
  out.close();

  std::ostringstream shell;
  shell << "cat \"" << cmd_file.string() << "\" | \"" << FLOWER_EXCHANGE_BIN
        << "\" >/dev/null 2>&1";

  const int rc = std::system(shell.str().c_str());
  if (WIFEXITED(rc)) {
    return WEXITSTATUS(rc);
  }
  return 128;
}

void assert_output_non_empty(const std::filesystem::path &output_path) {
  ASSERT_TRUE(std::filesystem::exists(output_path))
      << "Expected output file missing: " << output_path;

  std::ifstream in(output_path);
  std::string header;
  std::getline(in, header);
  ASSERT_FALSE(header.empty()) << "Output header is empty";
  ASSERT_NE(header.find("Order ID"), std::string::npos)
      << "Output header missing expected prefix";

  std::string first_row;
  std::getline(in, first_row);
  ASSERT_FALSE(first_row.empty()) << "Output has no execution rows";
}

std::string read_file_binary(const std::filesystem::path &output_path) {
  std::ifstream in(output_path, std::ios::binary);
  std::ostringstream buffer;
  buffer << in.rdbuf();
  return buffer.str();
}

std::size_t line_count(const std::string &text) {
  return static_cast<std::size_t>(std::count(text.begin(), text.end(), '\n'));
}

std::string drop_last_csv_column(const std::string &line) {
  const std::size_t pos = line.find_last_of(',');
  if (pos == std::string::npos) {
    return line;
  }
  return line.substr(0, pos);
}

std::string normalize_csv_without_timestamp(const std::string &text) {
  std::istringstream in(text);
  std::string line;
  std::string out;
  while (std::getline(in, line)) {
    out += drop_last_csv_column(line);
    out.push_back('\n');
  }
  return out;
}

} // namespace

TEST(StressLargeFiles, RootBigFixturePresentAndLarge) {
  const auto big_path = repo_path("big_orders.csv");

  ASSERT_TRUE(std::filesystem::exists(big_path));

  const auto big_size = std::filesystem::file_size(big_path);

  EXPECT_GT(big_size, 100ULL * 1024ULL * 1024ULL);
}

TEST(StressLargeFiles, ProcessBigFileSoftThroughput) {
  if (!env_flag_enabled("FLOWER_EXCHANGE_RUN_STRESS")) {
    GTEST_SKIP() << "Set FLOWER_EXCHANGE_RUN_STRESS=1 to execute large-file "
                    "stress runs.";
  }

  const auto big_path = repo_path("big_orders.csv");
  ASSERT_TRUE(std::filesystem::exists(big_path));

  const double budget_sec =
      env_double("FLOWER_EXCHANGE_STRESS_BIG_MAX_SECONDS", 900.0);
  const auto start = std::chrono::steady_clock::now();
  const int rc = run_process_once(big_path);
  const auto end = std::chrono::steady_clock::now();

  ASSERT_EQ(rc, 0);
  const double elapsed =
      std::chrono::duration_cast<std::chrono::duration<double>>(end - start)
          .count();

  assert_output_non_empty(output_path_for_input(big_path));
  EXPECT_LE(elapsed, budget_sec)
      << "Soft throughput regression: elapsed=" << elapsed
      << "s, budget=" << budget_sec << "s";
}

TEST(StressLargeFiles, BigFileByteDeterministicAcrossFiveRuns) {
  if (!env_flag_enabled("FLOWER_EXCHANGE_RUN_STRESS_DETERMINISM")) {
    GTEST_SKIP() << "Set FLOWER_EXCHANGE_RUN_STRESS_DETERMINISM=1 to run 5-run "
                    "byte checks.";
  }

  const auto big_path = repo_path("big_orders.csv");
  ASSERT_TRUE(std::filesystem::exists(big_path));

  std::vector<std::string> outputs;
  std::vector<std::size_t> line_counts;
  outputs.reserve(5);
  line_counts.reserve(5);

  for (int run = 0; run < 5; ++run) {
    const int rc = run_process_once(big_path);
    ASSERT_EQ(rc, 0) << "PROCESS failed on run=" << run;

    const auto output_path = output_path_for_input(big_path);
    assert_output_non_empty(output_path);

    const std::string bytes = read_file_binary(output_path);
    ASSERT_FALSE(bytes.empty());
    outputs.push_back(bytes);
    line_counts.push_back(line_count(bytes));
  }

  const std::string expected_header =
      "Order ID,Client Order Id,Instrument,Side,Exec "
      "Status,Quantity,Price,Reason,Timestamp\n";
  ASSERT_GE(outputs[0].size(), expected_header.size());
  EXPECT_EQ(outputs[0].substr(0, expected_header.size()), expected_header);

  for (std::size_t i = 1; i < line_counts.size(); ++i) {
    EXPECT_EQ(line_counts[i], line_counts[0])
        << "Output line-count changed between run 0 and run " << i;
  }

  for (std::size_t i = 0; i < outputs.size(); ++i) {
    for (std::size_t j = i + 1; j < outputs.size(); ++j) {
      EXPECT_EQ(normalize_csv_without_timestamp(outputs[i]),
                normalize_csv_without_timestamp(outputs[j]))
          << "Byte mismatch between runs " << i << " and " << j;
    }
  }
}
