add_executable(server main.cpp)

target_link_libraries(server
    PRIVATE sep_protocol
)

target_include_directories(server
    PRIVATE include
)