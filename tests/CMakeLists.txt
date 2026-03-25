add_executable(flower_exchange_tests
  test_mempool.cpp
  test_validator.cpp
  test_types.cpp
  test_csv_reader.cpp
  test_order_book.cpp
  test_matching_engine.cpp
  test_application_integration.cpp
)

target_link_libraries(flower_exchange_tests
  PRIVATE
    flower_exchange_lib
    GTest::gtest_main
)

target_compile_definitions(flower_exchange_tests
  PRIVATE
    FLOWER_EXCHANGE_BIN="$<TARGET_FILE:flower_exchange>"
    FLOWER_EXCHANGE_REPO_ROOT="${CMAKE_SOURCE_DIR}"
)

include(GoogleTest)
gtest_discover_tests(flower_exchange_tests
  DISCOVERY_MODE PRE_TEST
  DISCOVERY_TIMEOUT 60
)

add_executable(flower_exchange_stress_tests
  test_stress_large_files.cpp
)

target_link_libraries(flower_exchange_stress_tests
  PRIVATE
    flower_exchange_lib
    GTest::gtest_main
)

target_compile_definitions(flower_exchange_stress_tests
  PRIVATE
    FLOWER_EXCHANGE_BIN="$<TARGET_FILE:flower_exchange>"
    FLOWER_EXCHANGE_REPO_ROOT="${CMAKE_SOURCE_DIR}"
)

gtest_discover_tests(flower_exchange_stress_tests
  TEST_LIST FLOWER_EXCHANGE_STRESS_TESTS
  DISCOVERY_MODE PRE_TEST
  DISCOVERY_TIMEOUT 60
  PROPERTIES LABELS "stress;manual"
)
