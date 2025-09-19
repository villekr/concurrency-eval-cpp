# Concurrency Evaluation - C++
C++ code for [How Do You Like Your Lambda Concurrency](https://ville-karkkainen.medium.com/how-do-you-like-your-aws-lambda-concurrency-part-1-introduction-7a3f7ecfe4b5)-blog series.

# Requirements
* C++23

The target is to implement the following pseudocode as effectively as possible using language-specific idioms and constrains to achieve concurrency/parallelism.
Mandatory requirements:
- Code must contain the following three constructs: 
  - handler: Language-specific AWS Lambda handler or equivalent entrypoint
  - processor: List objects from a specified S3 bucket and process them concurrently/parallel
  - get: Get a single object's body from S3, try to find a string if specified
- The processor-function must be encapsulated with timing functions
- S3 bucket will contain at maximum 1000 objects
- Each S3 objects' body must be fully read
- Code must return at least the following attributes as lambda handler response:
  - time (float): duration as float in seconds rounded to one decimal place
  - result (string): If find-string is specified, then the key of the first s3 object that contains that string (or None). Otherwise, the number of s3 objects listed
```
func handler(event):
    timer.start()
    result = processor(event)
    timer.stop()
    return {
        "time": timer.elapsed,
        "result": result
    }
    
func processor(event):
    s3_objects = aws_sdk.list_objects(s3_bucket)
    results = [get(s3_key, event[find]) for s3_objects]
    return first_non_none(results) if event[find] else str(len(s3_objects))

func get(s3_key, find):
    body = aws_sdk.get_object(s3_key).body
    return body.find(find) if find else None
```

## Docker builder image

GitHub action used docker image for building. Build and push to Docker Hub if any changes are made to the Dockerfile.
```
docker build -f docker/Dockerfile -t villekr/concurrency-eval-cpp-build-container:0.3 .
docker push villekr/concurrency-eval-cpp-build-container:0.3
```

## Build deployment package locally

```
docker run -it --rm -v $(pwd):/builder villekr/concurrency-eval-cpp-build-container:0.3 bash
cd builder && mkdir build && cd build
cmake ..  \
    -DCMAKE_PREFIX_PATH=/vcpkg/installed/arm64-linux/ \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_INSTALL_PREFIX=/out \
    -DCMAKE_CXX_STANDARD=23 \
    -DCMAKE_CXX_COMPILER=g++ \
    -DENABLE_LTO=ON \
    -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=TRUE \
    -DCMAKE_CXX_FLAGS_RELEASE="-O3 -DNDEBUG -ffunction-sections -fdata-sections -flto" \
    -DCMAKE_EXE_LINKER_FLAGS_RELEASE="-Wl,--gc-sections -Wl,--as-needed"    
make
make aws-lambda-package-concurrency-eval-cpp
```

Note:
- Local container build writes the ZIP to /builder/build inside the container. On the host, the same file appears at ./build/concurrency-eval-cpp.zip.
- In GitHub Actions, the build runs inside the container at /github/workspace/build; on the runner host it is available at ${GITHUB_WORKSPACE}/build/concurrency-eval-cpp.zip. The upload step must use the host path (${GITHUB_WORKSPACE}), not the container path (/github/workspace).

