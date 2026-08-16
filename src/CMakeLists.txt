add_library(common INTERFACE)
target_include_directories(common INTERFACE ${CMAKE_SOURCE_DIR}/src)

add_library(flower_exchange_lib STATIC
  app/application.cpp
  app/file_processing_pipeline.cpp
  app/processing_reporter.cpp
  matching/bitmask_order_book.cpp
  matching/map_order_book.cpp
  matching/matching_engine.cpp
  matching/order_processor.cpp
  io/csv_reader.cpp
  io/csv_order_producer.cpp
  io/csv_report_writer.cpp
  common/types.cpp
  common/timestamp_cache.cpp
)
target_include_directories(flower_exchange_lib PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(flower_exchange_lib PUBLIC common Boost::boost pthread)

add_executable(flower_exchange main.cpp)
target_link_libraries(flower_exchange PRIVATE flower_exchange_lib)
