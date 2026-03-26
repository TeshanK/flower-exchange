#include <gtest/gtest.h>
#include <sys/wait.h>

#include <algorithm>
#include <array>
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

int run_with_commands(const std::vector<std::string> &commands) {
  const auto tmp_dir = std::filesystem::temp_directory_path();
  const auto cmd_file = tmp_dir / "flower_exchange_cmds.txt";

  std::ofstream out(cmd_file);
  for (const auto &cmd : commands) {
    out << cmd << '\n';
  }
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

std::vector<std::string> read_lines(const std::filesystem::path &file_path) {
  std::ifstream in(file_path);
  std::vector<std::string> lines;
  std::string line;
  while (std::getline(in, line)) {
    lines.push_back(line);
  }
  return lines;
}

std::string read_file_text(const std::filesystem::path &file_path) {
  std::ifstream in(file_path, std::ios::binary);
  std::ostringstream buffer;
  buffer << in.rdbuf();
  return buffer.str();
}

int count_char(const std::string &text, char ch) {
  return static_cast<int>(std::count_if(
      text.begin(), text.end(), [ch](char c) -> bool { return c == ch; }));
}

std::filesystem::path find_output_file(const std::string &filename) {
  std::filesystem::path cwd_candidate =
      std::filesystem::current_path() / "output" / filename;
  if (std::filesystem::exists(cwd_candidate)) {
    return cwd_candidate;
  }

  std::filesystem::path repo_candidate =
      std::filesystem::path("..") / "output" / filename;
  if (std::filesystem::exists(repo_candidate)) {
    return repo_candidate;
  }

  return cwd_candidate;
}

std::filesystem::path repo_path(const std::string &rel) {
  return std::filesystem::path(FLOWER_EXCHANGE_REPO_ROOT) / rel;
}

std::vector<std::string> split_csv_simple(const std::string &line) {
  std::vector<std::string> cols;
  std::string current;
  for (char c : line) {
    if (c == ',') {
      cols.push_back(current);
      current.clear();
    } else {
      current.push_back(c);
    }
  }
  cols.push_back(current);
  return cols;
}

bool is_valid_timestamp_field(const std::string &ts) {
  if (ts.size() != 19) {
    return false;
  }
  constexpr std::array<int, 17> digit_positions = {
      0, 1, 2, 3, 4, 5, 6, 7, 9, 10, 11, 12, 13, 14, 16, 17, 18,
  };
  const bool all_digits = std::all_of(
      digit_positions.begin(), digit_positions.end(), [&](int idx) -> bool {
        const char c = ts[static_cast<std::size_t>(idx)];
        return c >= '0' && c <= '9';
      });
  if (!all_digits) {
    return false;
  }
  return ts[8] == '-' && ts[15] == '.';
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

TEST(ApplicationIntegrationTest, MissingInputDoesNotCrashProcessLoop) {
  const int rc = run_with_commands({
      "PROCESS " + repo_path("input_files/does_not_exist.csv").string(),
      "QUIT",
  });
  EXPECT_EQ(rc, 0);
}

TEST(ApplicationIntegrationTest, RepeatedProcessResetsOrderSequence) {
  const int rc = run_with_commands({
      "PROCESS " + repo_path("input_files/example3_orders.csv").string(),
      "PROCESS " + repo_path("input_files/example3_orders.csv").string(),
      "QUIT",
  });
  ASSERT_EQ(rc, 0);

  const auto lines =
      read_lines(find_output_file("example3_orders_reports.csv"));
  ASSERT_GE(lines.size(), 2u);
  EXPECT_NE(lines[1].find("ord1"), std::string::npos);
}

TEST(ApplicationIntegrationTest, MalformedNumericInputFailsProcess) {
  std::filesystem::create_directories(repo_path("input_files"));
  const std::filesystem::path bad_file =
      repo_path("input_files/malformed_numeric_orders.csv");

  {
    std::ofstream out(bad_file);
    out << "Client Order ID,Instrument,Side,Quantity,Price\n";
    out << "aa13,Rose,abc,100,55.00\n";
  }

  const int rc = run_with_commands({
      "PROCESS " + bad_file.string(),
      "QUIT",
  });

  EXPECT_NE(rc, 0);
}

TEST(ApplicationIntegrationTest, MultipleSequentialRunsRemainDeterministic) {
  std::vector<std::string> commands;
  commands.reserve(51);
  for (int i = 0; i < 50; ++i) {
    commands.push_back("PROCESS " +
                       repo_path("input_files/example5_orders.csv").string());
  }
  commands.emplace_back("QUIT");

  const int rc = run_with_commands(commands);
  ASSERT_EQ(rc, 0);

  const auto lines =
      read_lines(find_output_file("example5_orders_reports.csv"));
  ASSERT_EQ(lines.size(), 7u);
  EXPECT_NE(lines[1].find("ord1"), std::string::npos);
  EXPECT_NE(lines.back().find("ord1"), std::string::npos);
}

TEST(ApplicationIntegrationTest, ExampleFixturesMatchCanonicalOutput) {
  const std::vector<std::vector<std::string>> expected_rows = {
      {
          "Order ID,Client Order Id,Instrument,Side,Exec "
          "Status,Quantity,Price,Reason,Timestamp",
          "ord1,aa13,Rose,2,New,100,55.00,,",
      },
      {
          "Order ID,Client Order Id,Instrument,Side,Exec "
          "Status,Quantity,Price,Reason,Timestamp",
          "ord1,aa13,Rose,2,New,100,55.00,,",
          "ord2,aa14,Rose,2,New,100,45.00,,",
          "ord3,aa15,Rose,1,New,100,35.00,,",
      },
      {
          "Order ID,Client Order Id,Instrument,Side,Exec "
          "Status,Quantity,Price,Reason,Timestamp",
          "ord1,aa13,Rose,2,New,100,55.00,,",
          "ord2,aa14,Rose,2,New,100,45.00,,",
          "ord3,aa15,Rose,1,Fill,100,45.00,,",
          "ord2,aa14,Rose,2,Fill,100,45.00,,",
      },
      {
          "Order ID,Client Order Id,Instrument,Side,Exec "
          "Status,Quantity,Price,Reason,Timestamp",
          "ord1,aa13,Rose,2,New,100,55.00,,",
          "ord2,aa14,Rose,2,New,100,45.00,,",
          "ord3,aa15,Rose,1,Pfill,100,45.00,,",
          "ord2,aa14,Rose,2,Fill,100,45.00,,",
      },
      {
          "Order ID,Client Order Id,Instrument,Side,Exec "
          "Status,Quantity,Price,Reason,Timestamp",
          "ord1,aa13,Rose,1,New,100,55.00,,",
          "ord2,aa14,Rose,1,New,100,65.00,,",
          "ord3,aa15,Rose,2,Pfill,100,65.00,,",
          "ord2,aa14,Rose,1,Fill,100,65.00,,",
          "ord3,aa15,Rose,2,Pfill,100,55.00,,",
          "ord1,aa13,Rose,1,Fill,100,55.00,,",
      },
      {
          "Order ID,Client Order Id,Instrument,Side,Exec "
          "Status,Quantity,Price,Reason,Timestamp",
          "ord1,aa13,Rose,1,New,100,55.00,,",
          "ord2,aa14,Rose,1,New,100,65.00,,",
          "ord3,aa15,Rose,2,Pfill,100,65.00,,",
          "ord2,aa14,Rose,1,Fill,100,65.00,,",
          "ord3,aa15,Rose,2,Pfill,100,55.00,,",
          "ord1,aa13,Rose,1,Fill,100,55.00,,",
          "ord4,aa16,Rose,1,Fill,100,1.00,,",
          "ord3,aa15,Rose,2,Fill,100,1.00,,",
      },
      {
          "Order ID,Client Order Id,Instrument,Side,Exec "
          "Status,Quantity,Price,Reason,Timestamp",
          "ord1,aa13,,1,Reject,100,55.00,Invalid instrument,",
          "ord2,aa14,Rose,3,Reject,100,65.00,Invalid side,",
          "ord3,aa15,Lavender,2,Reject,101,1.00,Invalid size,",
          "ord4,aa16,Tulip,1,Reject,100,-1.00,Invalid price,",
          "ord5,aa17,Orchid,1,Reject,1000,-1.00,Invalid price,",
      },
      {
          "Order ID,Client Order Id,Instrument,Side,Exec "
          "Status,Quantity,Price,Reason,Timestamp",
          "ord1,aa12,Rose,2,New,200,45.00,,",
          "ord2,aa13,Rose,2,New,200,45.00,,",
          "ord3,aa14,Rose,1,Fill,100,45.00,,",
          "ord1,aa12,Rose,2,Pfill,100,45.00,,",
          "ord4,aa15,Rose,1,Fill,100,45.00,,",
          "ord1,aa12,Rose,2,Fill,100,45.00,,",
      },
  };

  for (int i = 1; i <= 8; ++i) {
    const std::string input_name =
        "input_files/example" + std::to_string(i) + "_orders.csv";
    const std::string output_name =
        "example" + std::to_string(i) + "_orders_reports.csv";

    const int rc = run_with_commands({
        "PROCESS " + repo_path(input_name).string(),
        "QUIT",
    });
    ASSERT_EQ(rc, 0) << "PROCESS failed for fixture " << input_name;

    const auto actual_lines = read_lines(find_output_file(output_name));
    ASSERT_EQ(actual_lines.size(),
              expected_rows[static_cast<std::size_t>(i - 1)].size())
        << "Row count mismatch for fixture " << input_name;

    for (std::size_t row = 0; row < actual_lines.size(); ++row) {
      EXPECT_EQ(drop_last_csv_column(actual_lines[row]),
                drop_last_csv_column(
                    expected_rows[static_cast<std::size_t>(i - 1)][row]))
          << "Output mismatch (excluding timestamp) for fixture " << input_name
          << " row=" << row;

      if (row > 0) {
        auto cols = split_csv_simple(actual_lines[row]);
        ASSERT_EQ(cols.size(), 9u);
        EXPECT_TRUE(is_valid_timestamp_field(cols[8]))
            << "Invalid timestamp format for fixture " << input_name
            << " row=" << row << " value=" << cols[8];
      }
    }
  }
}

TEST(ApplicationIntegrationTest, RepeatedRunOutputIsByteStable) {
  const std::string input_name = "input_files/example7_orders.csv";
  const std::string output_name = "example7_orders_reports.csv";

  const int first_rc = run_with_commands({
      "PROCESS " + repo_path(input_name).string(),
      "QUIT",
  });
  ASSERT_EQ(first_rc, 0);

  const std::string first_output =
      read_file_text(find_output_file(output_name));
  ASSERT_FALSE(first_output.empty());

  const int second_rc = run_with_commands({
      "PROCESS " + repo_path(input_name).string(),
      "QUIT",
  });
  ASSERT_EQ(second_rc, 0);

  const std::string second_output =
      read_file_text(find_output_file(output_name));
  EXPECT_EQ(normalize_csv_without_timestamp(second_output),
            normalize_csv_without_timestamp(first_output));
}

TEST(ApplicationIntegrationTest,
     AllExampleOutputsAreByteStableAcrossFreshProcesses) {
  for (int i = 1; i <= 8; ++i) {
    const std::string input_name =
        "input_files/example" + std::to_string(i) + "_orders.csv";
    const std::string output_name =
        "example" + std::to_string(i) + "_orders_reports.csv";

    const int first_rc = run_with_commands({
        "PROCESS " + repo_path(input_name).string(),
        "QUIT",
    });
    ASSERT_EQ(first_rc, 0);
    const std::string first_output =
        read_file_text(find_output_file(output_name));
    ASSERT_FALSE(first_output.empty());

    const int second_rc = run_with_commands({
        "PROCESS " + repo_path(input_name).string(),
        "QUIT",
    });
    ASSERT_EQ(second_rc, 0);
    const std::string second_output =
        read_file_text(find_output_file(output_name));

    EXPECT_EQ(normalize_csv_without_timestamp(second_output),
              normalize_csv_without_timestamp(first_output))
        << "Byte-level determinism mismatch for fixture " << input_name;
  }
}

TEST(ApplicationIntegrationTest, AllExampleOutputsKeepStableCsvShape) {
  const std::string expected_header =
      "Order ID,Client Order Id,Instrument,Side,Exec "
      "Status,Quantity,Price,Reason,Timestamp";

  for (int i = 1; i <= 8; ++i) {
    const std::string input_name =
        "input_files/example" + std::to_string(i) + "_orders.csv";
    const std::string output_name =
        "example" + std::to_string(i) + "_orders_reports.csv";

    const int rc = run_with_commands({
        "PROCESS " + repo_path(input_name).string(),
        "QUIT",
    });
    ASSERT_EQ(rc, 0);

    const auto lines = read_lines(find_output_file(output_name));
    ASSERT_FALSE(lines.empty());
    EXPECT_EQ(lines.front(), expected_header);

    for (std::size_t row = 1; row < lines.size(); ++row) {
      EXPECT_EQ(count_char(lines[row], ','), 8)
          << "Unexpected column count in " << output_name << " row=" << row;

      auto cols = split_csv_simple(lines[row]);
      ASSERT_EQ(cols.size(), 9u);
      EXPECT_TRUE(is_valid_timestamp_field(cols[8]))
          << "Invalid timestamp format in " << output_name << " row=" << row;
    }

    const std::string bytes = read_file_text(find_output_file(output_name));
    ASSERT_FALSE(bytes.empty());
    EXPECT_EQ(bytes.back(), '\n')
        << "Output should end with newline for " << output_name;
  }
}

TEST(ApplicationIntegrationTest, RejectedOrdersProduceExactlyOneRejectRowEach) {
  const std::filesystem::path input_path =
      repo_path("input_files/reject_reason_matrix_orders.csv");
  {
    std::ofstream out(input_path);
    out << "Client Order ID,Instrument,Side,Quantity,Price\n";
    out << "r1,,1,100,55.00\n";
    out << "r2,Rose,3,100,55.00\n";
    out << "r3,Rose,1,15,55.00\n";
    out << "r4,Rose,1,100,-1.00\n";
  }

  const int rc = run_with_commands({
      "PROCESS " + input_path.string(),
      "QUIT",
  });
  ASSERT_EQ(rc, 0);

  const auto lines =
      read_lines(find_output_file("reject_reason_matrix_orders_reports.csv"));
  ASSERT_EQ(lines.size(), 5u);

  for (std::size_t row = 1; row < lines.size(); ++row) {
    auto cols = split_csv_simple(lines[row]);
    ASSERT_EQ(cols.size(), 9u);
    EXPECT_EQ(cols[4], "Reject");
    EXPECT_FALSE(cols[7].empty());
    EXPECT_TRUE(is_valid_timestamp_field(cols[8]));
  }

  EXPECT_NE(lines[1].find("Invalid instrument"), std::string::npos);
  EXPECT_NE(lines[2].find("Invalid side"), std::string::npos);
  EXPECT_NE(lines[3].find("Invalid size"), std::string::npos);
  EXPECT_NE(lines[4].find("Invalid price"), std::string::npos);
}

TEST(ApplicationIntegrationTest, OrderIdsProgressAfterRejection) {
  const std::filesystem::path input_path =
      repo_path("input_files/reject_then_valid_orders.csv");
  {
    std::ofstream out(input_path);
    out << "Client Order ID,Instrument,Side,Quantity,Price\n";
    out << "x1,,1,100,55.00\n";
    out << "x2,Rose,2,100,55.00\n";
    out << "x3,Rose,1,100,55.00\n";
  }

  const int rc = run_with_commands({
      "PROCESS " + input_path.string(),
      "QUIT",
  });
  ASSERT_EQ(rc, 0);

  const auto lines =
      read_lines(find_output_file("reject_then_valid_orders_reports.csv"));
  ASSERT_EQ(lines.size(), 5u);

  EXPECT_NE(lines[1].find("ord1"), std::string::npos);
  EXPECT_NE(lines[1].find(",Reject,"), std::string::npos);

  bool found_ord2 = false;
  bool found_ord3 = false;
  for (std::size_t row = 2; row < lines.size(); ++row) {
    if (lines[row].find("ord2") != std::string::npos) {
      found_ord2 = true;
    }
    if (lines[row].find("ord3") != std::string::npos) {
      found_ord3 = true;
    }
  }
  EXPECT_TRUE(found_ord2);
  EXPECT_TRUE(found_ord3);
}

TEST(ApplicationIntegrationTest,
     OversizedAndNonFinitePricesAreRejectedWithoutCrash) {
  const std::filesystem::path input_path =
      repo_path("input_files/price_overflow_orders.csv");
  {
    std::ofstream out(input_path);
    out << "Client Order ID,Instrument,Side,Quantity,Price\n";
    out << "p1,Rose,1,100,1000000000000.00\n";
    out << "p2,Rose,1,100,1e309\n";
  }

  const int rc = run_with_commands({
      "PROCESS " + input_path.string(),
      "QUIT",
  });
  ASSERT_EQ(rc, 0);

  const auto lines =
      read_lines(find_output_file("price_overflow_orders_reports.csv"));
  ASSERT_EQ(lines.size(), 3u);

  for (std::size_t row = 1; row < lines.size(); ++row) {
    auto cols = split_csv_simple(lines[row]);
    ASSERT_EQ(cols.size(), 9u);
    EXPECT_EQ(cols[4], "Reject");
    EXPECT_EQ(cols[7], "Invalid price");
    EXPECT_TRUE(is_valid_timestamp_field(cols[8]));
  }
}
