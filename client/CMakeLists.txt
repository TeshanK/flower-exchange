set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g -O0")

add_executable(client main.cpp)

target_link_libraries(client
        PRIVATE sep_protocol
        PRIVATE common
        PRIVATE network
)