# Concurrency Evaluation - C++
C++ code for [How Do You Like Your Lambda Concurrency](https://medium.com/@ville-karkkainen/how-do-you-like-your-lambda-concurrency-part-i-introduction-7a3f7ecfe4b5)-blog series.

# Requirements
* C++
* Docker

# Build Deployment Package

```
docker build -f docker/Dockerfile -t concurrency-eval-cpp .
docker run -it --rm -v $(pwd):/app concurrency-eval-cpp bash
mkdir build && cd build
cmake ..  \
    -DCMAKE_PREFIX_PATH=/vcpkg/installed/arm64-linux/ \
    -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_INSTALL_PREFIX=/out -DCMAKE_CXX_COMPILER=g++
make
make aws-lambda-package-concurrency-eval-cpp
```