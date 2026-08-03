set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g -O0")

add_executable(server main.cpp)

target_link_libraries(server
        PRIVATE sep_protocol
        PRIVATE common
        PRIVATE network
)

target_include_directories(server
        PRIVATE include
)

target_compile_options(server PRIVATE -Wall -Wextra -Werror)