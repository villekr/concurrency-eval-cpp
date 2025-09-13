cmake_minimum_required(VERSION 3.20)
project(concurrency-eval-cpp LANGUAGES CXX)

# Enforce latest C++ standard per requirements
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
          $<$<CONFIG:Release>:-O3 -DNDEBUG>)
elseif(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4)
endif()

# Require C++23 features
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

aws_lambda_package_target(${PROJECT_NAME})

# run_local
add_executable(run_local "src/run_local.cpp" include/lambda_handler.h src/lambda_handler.cpp)
target_link_libraries(run_local PRIVATE AWS::aws-lambda-runtime ${AWSSDK_LINK_LIBRARIES})
