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

## Build deployment package locally

```
docker build -f docker/Dockerfile -t concurrency-eval-cpp .
docker run -it --rm -v $(pwd):/app concurrency-eval-cpp bash
cd /app && mkdir build && cd build
cmake ..  \
    -DCMAKE_PREFIX_PATH=/vcpkg/installed/arm64-linux/ \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_INSTALL_PREFIX=/out \
    -DCMAKE_CXX_STANDARD=23 \
    -DCMAKE_CXX_COMPILER=g++
make
make aws-lambda-package-concurrency-eval-cpp
```
