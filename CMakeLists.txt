cmake_minimum_required(VERSION 3.9)
set(CMAKE_CXX_STANDARD 11)
project(concurrency-eval-cpp LANGUAGES CXX)

# Enforce latest C++ standard per requirements
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optimization options
option(ENABLE_LTO "Enable Link Time Optimization (IPO) in Release builds" ON)
option(ENABLE_NATIVE_OPTIMIZATIONS "Enable -march=native/-mtune=native for GNU/Clang when not cross-compiling" ON)

find_package(AWSSDK COMPONENTS s3)
find_package(aws-lambda-runtime)

add_executable(${PROJECT_NAME} "src/main.cpp" include/lambda_handler.h src/lambda_handler.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE AWS::aws-lambda-runtime ${AWSSDK_LINK_LIBRARIES})

# Warnings and optimizations suitable for release builds
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
  target_compile_options(${PROJECT_NAME} PRIVATE
          -Wall -Wextra -Wconversion -Wshadow -Wno-sign-conversion
          -ffunction-sections -fdata-sections
          $<$<CONFIG:Release>:-O3 -DNDEBUG>)
  target_link_options(${PROJECT_NAME} PRIVATE
          $<$<CONFIG:Release>:-Wl,--gc-sections -Wl,--as-needed>)
  if(ENABLE_NATIVE_OPTIMIZATIONS AND NOT CMAKE_CROSSCOMPILING)
    target_compile_options(${PROJECT_NAME} PRIVATE -march=native -mtune=native)
  endif()
elseif(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4)
endif()

# Require C++23 features
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

# Enable IPO/LTO if supported
if(ENABLE_LTO AND CMAKE_BUILD_TYPE STREQUAL "Release")
  include(CheckIPOSupported)
  check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
  if(ipo_supported)
    set_property(TARGET ${PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  else()
    message(STATUS "IPO/LTO not supported: ${ipo_error}")
  endif()
endif()

aws_lambda_package_target(${PROJECT_NAME})

# run_local
add_executable(run_local "src/run_local.cpp" include/lambda_handler.h src/lambda_handler.cpp)
target_link_libraries(run_local PRIVATE AWS::aws-lambda-runtime ${AWSSDK_LINK_LIBRARIES})

# Propagate the same compile/link settings to run_local
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
  target_compile_options(run_local PRIVATE
          -Wall -Wextra -Wconversion -Wshadow -Wno-sign-conversion
          -ffunction-sections -fdata-sections
          $<$<CONFIG:Release>:-O3 -DNDEBUG>)
  target_link_options(run_local PRIVATE
          $<$<CONFIG:Release>:-Wl,--gc-sections -Wl,--as-needed>)
  if(ENABLE_NATIVE_OPTIMIZATIONS AND NOT CMAKE_CROSSCOMPILING)
    target_compile_options(run_local PRIVATE -march=native -mtune=native)
  endif()
elseif(MSVC)
  target_compile_options(run_local PRIVATE /W4)
endif()

if(ENABLE_LTO AND CMAKE_BUILD_TYPE STREQUAL "Release")
  include(CheckIPOSupported)
  check_ipo_supported(RESULT ipo_supported_run_local OUTPUT ipo_error_run_local)
  if(ipo_supported_run_local)
    set_property(TARGET run_local PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  else()
    message(STATUS "IPO/LTO not supported for run_local: ${ipo_error_run_local}")
  endif()
endif()