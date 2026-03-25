include(FetchContent)

find_package(Boost QUIET)
if(NOT Boost_FOUND)
  message(STATUS "Boost not found locally. Fetching Boost headers...")
  FetchContent_Declare(
    boost
    URL https://github.com/boostorg/boost/releases/download/boost-1.87.0/boost-1.87.0-cmake.tar.xz
  )
  FetchContent_MakeAvailable(boost)

  if(NOT TARGET Boost::boost)
    add_library(boost_headers INTERFACE)
    target_include_directories(boost_headers INTERFACE ${boost_SOURCE_DIR})
    add_library(Boost::boost ALIAS boost_headers)
  endif()
endif()

find_package(GTest QUIET)
if(NOT GTest_FOUND)
  message(STATUS "GoogleTest not found locally. Fetching GoogleTest...")
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  )
  FetchContent_MakeAvailable(googletest)
endif()
