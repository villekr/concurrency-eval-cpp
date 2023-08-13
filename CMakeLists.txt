cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 11)
project(concurrency-eval-cpp LANGUAGES CXX)

find_package(AWSSDK COMPONENTS s3)
find_package(aws-lambda-runtime)

add_executable(${PROJECT_NAME} "src/main.cpp" include/lambda_handler.h src/lambda_handler.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE AWS::aws-lambda-runtime ${AWSSDK_LINK_LIBRARIES})

target_compile_options(${PROJECT_NAME} PRIVATE
        "-Wall"
        "-Wextra"
        "-Wconversion"
        "-Wshadow"
        "-Wno-sign-conversion")

target_compile_features(${PROJECT_NAME} PRIVATE "cxx_std_11")

aws_lambda_package_target(${PROJECT_NAME})

# run_local
add_executable(run_local "src/run_local.cpp" include/lambda_handler.h src/lambda_handler.cpp)
target_link_libraries(run_local PRIVATE AWS::aws-lambda-runtime ${AWSSDK_LINK_LIBRARIES})
