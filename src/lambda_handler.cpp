#include "../include/lambda_handler.h"
#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/platform/Environment.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/lambda-runtime/runtime.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/ListObjectsV2Request.h>
#include <chrono>
#include <cmath>
#include <future>
#include <thread>
#include <vector>

using namespace std;
using namespace aws::lambda_runtime;
using namespace aws::lambda_runtime;
using namespace Aws::Utils::Logging;
using namespace Aws::Utils::Json;
using namespace Aws::S3::Model;
using namespace Aws::S3;

string get(S3Client& s3_client, const Aws::String& object_key, const Aws::String& from_bucket, const string& find)
{
    GetObjectRequest object_request;
    object_request.WithBucket(from_bucket).WithKey(object_key);

    auto get_object_outcome = s3_client.GetObject(object_request);

    if (get_object_outcome.IsSuccess()) {
        auto& body_stream = get_object_outcome.GetResult().GetBody();
        stringstream ss;
        ss << body_stream.rdbuf();

        if (!find.empty()) {
            std::string body = ss.str();
            std::size_t found = body.find(find);
            if (found != std::string::npos) {
                return object_key.c_str();
            }
        }

        return {};
    }

    throw std::runtime_error(
        string("GetObject error: ") + get_object_outcome.GetError().GetExceptionName() + " " +
        get_object_outcome.GetError().GetMessage());
}

string processor(invocation_request const& req)
{
    JsonValue const json(req.payload);

    auto v = json.View();
    auto bucket_name = v.GetString("s3_bucket_name");
    auto folder = v.GetString("folder");
    auto find = v.GetString("find");

    Aws::Client::ClientConfiguration config;
    config.region = Aws::Environment::GetEnv("AWS_REGION");
    config.caFile = Aws::Environment::GetEnv("CA_BUNDLE_FILEPATH");

    S3Client s3_client(config);
    ListObjectsV2Request objects_request;
    objects_request.WithBucket(bucket_name).WithPrefix(folder);

    auto list_objects_outcome = s3_client.ListObjectsV2(objects_request);

    if (list_objects_outcome.IsSuccess()) {
        auto objects = list_objects_outcome.GetResult().GetContents();
        vector<future<string>> futures;
        vector<string> responses;
        responses.resize(objects.size());

        for (const auto& object : objects) {
            auto object_key = object.GetKey();
            futures.push_back(async(launch::async, [bucket_name, object_key, &s3_client, find]() {
                return get(s3_client, object_key, bucket_name, find);
            }));
        }

        for (size_t i = 0; i < objects.size(); ++i) {
            responses[i] = futures[i].get();
        }

        if (!find.empty()) {
            for (const auto& response : responses) {
                if (!response.empty()) {
                    return response;
                }
            }
        }

        return to_string(responses.size());
    }

    throw runtime_error(
        string("ListObjects error: ") + list_objects_outcome.GetError().GetExceptionName() + " " +
        list_objects_outcome.GetError().GetMessage());
}

invocation_response lambda_handler(invocation_request const& req)
{
    auto start = chrono::high_resolution_clock::now();
    auto responses = processor(req);
    auto finish = chrono::high_resolution_clock::now();
    chrono::duration<double> const elapsed = finish - start;

    JsonValue json_response;
    json_response.WithString("lang", "C++");
    json_response.WithString("detail", "aws-sdk-cpp");
    json_response.WithString("result", responses);
    json_response.WithDouble("time", round(elapsed.count() * 10.0) / 10);

    Aws::String const response_payload = json_response.View().WriteCompact();

    return invocation_response::success(response_payload, "application/json");
}