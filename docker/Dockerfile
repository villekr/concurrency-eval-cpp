FROM ubuntu:latest

ENV TZ=Europe/Helsinki
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt-get update && apt-get -y install build-essential cmake git curl unzip tar zip ninja-build linux-libc-dev pkg-config libcurl4-openssl-dev linux-libc-dev

# Install vcpkg
RUN git clone https://github.com/microsoft/vcpkg
RUN ./vcpkg/bootstrap-vcpkg.sh

# Install aws-sdk-cpp s3 package
RUN VCPKG_FORCE_SYSTEM_BINARIES=1 ./vcpkg/vcpkg install "aws-sdk-cpp[s3]" --recurse

# Build aws-lambda-cpp
RUN git clone https://github.com/awslabs/aws-lambda-cpp.git
RUN cd aws-lambda-cpp && mkdir build
WORKDIR /aws-lambda-cpp/build

RUN cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_INSTALL_PREFIX=/out -DCMAKE_CXX_COMPILER=g++
RUN make && make install

CMD ["bash"]