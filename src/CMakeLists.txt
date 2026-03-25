add_library(common INTERFACE)
target_include_directories(common INTERFACE ${CMAKE_SOURCE_DIR}/src)

add_library(flower_exchange_lib STATIC
  app/application.cpp
  matching/order_book.cpp
  matching/matching_engine.cpp
  io/csv_reader.cpp
  common/types.cpp
)
target_include_directories(flower_exchange_lib PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(flower_exchange_lib PUBLIC common Boost::boost pthread)

add_executable(flower_exchange main.cpp)
target_link_libraries(flower_exchange PRIVATE flower_exchange_lib)
