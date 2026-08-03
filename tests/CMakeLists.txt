find_package(GTest)

include(GoogleTest)

function(add_gtest TEST_NAME)
    add_executable(${TEST_NAME}
            ${ARGN}
    )

    target_link_libraries(${TEST_NAME}
            PRIVATE
            sep_protocol
            common
            GTest::gtest_main
    )

    gtest_discover_tests(${TEST_NAME})
endfunction()

add_gtest(test_sequencer test_sequencer.cpp)
add_gtest(test_map_orderbook test_map_orderbook.cpp)
add_gtest(test_matching_engine test_matching_engine.cpp)