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
#include <algorithm>
#include <chrono>
#include <cmath>
#include <future>
#include <thread>
#include <vector>
#include <limits>

using namespace std;
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

        if (find.empty()) {
            // Fully consume the stream but do not store it to minimize memory footprint
            char buffer[8192];
            while (body_stream.read(buffer, sizeof(buffer)) || body_stream.gcount() > 0) {
                // discard
            }
            return {};
        }

        // Build a single std::string (no intermediate stringstream copy). Reserve if ContentLength is known.
        std::string body;
        auto len = get_object_outcome.GetResult().GetContentLength();
        if (len > 0 && static_cast<unsigned long long>(len) < std::numeric_limits<size_t>::max()) {
            body.reserve(static_cast<size_t>(len));
        }
        body.assign(std::istreambuf_iterator<char>(body_stream), std::istreambuf_iterator<char>());

        if (!body.empty()) {
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

    // Determine concurrency limit: env MAX_CONCURRENCY or a sensible default for I/O-bound workloads
    unsigned int hw = std::thread::hardware_concurrency();
    unsigned int default_limit = std::min(32u, std::max(4u, hw ? 2u * hw : 8u));
    unsigned int concurrency_limit = default_limit;
    {
        auto env_val = Aws::Environment::GetEnv("MAX_CONCURRENCY");
        if (!env_val.empty()) {
            try {
                unsigned long parsed_limit = std::stoul(env_val.c_str());
                if (parsed_limit >= 1 && parsed_limit <= 256) {
                    concurrency_limit = static_cast<unsigned int>(parsed_limit);
                }
            }
            catch (...) {
                // ignore parse errors, keep default
            }
        }
    }

    Aws::Client::ClientConfiguration config;
    config.region = Aws::Environment::GetEnv("AWS_REGION");
    config.caFile = Aws::Environment::GetEnv("CA_BUNDLE_FILEPATH");
    config.maxConnections = static_cast<long>(concurrency_limit); // align HTTP pool with our concurrency

    S3Client s3_client(config);
    ListObjectsV2Request objects_request;
    objects_request.WithBucket(bucket_name).WithPrefix(folder);

    auto list_objects_outcome = s3_client.ListObjectsV2(objects_request);

    if (list_objects_outcome.IsSuccess()) {
        auto objects = list_objects_outcome.GetResult().GetContents();

        const bool do_find = !find.empty();
        size_t n = objects.size();

        // Track earliest matching key by list order without keeping all responses
        size_t best_index = std::numeric_limits<size_t>::max();
        std::string best_key;

        // Process in bounded batches to limit threads and memory
        for (size_t start = 0; start < n; start += concurrency_limit) {
            size_t end = std::min(n, start + concurrency_limit);

            std::vector<std::future<std::string>> futures;
            futures.reserve(end - start);

            for (size_t i = start; i < end; ++i) {
                auto object_key = objects[i].GetKey();
                futures.push_back(std::async(std::launch::async, [bucket_name, object_key, &s3_client, find]() {
                    return get(s3_client, object_key, bucket_name, find);
                }));
            }

            // Collect in list order for deterministic "first" selection
            for (size_t i = start; i < end; ++i) {
                std::string r = futures[i - start].get();
                if (do_find && !r.empty() && i < best_index) {
                    best_index = i;
                    best_key = std::move(r);
                }
            }
        }

        if (do_find && best_index != std::numeric_limits<size_t>::max()) {
            return best_key;
        }

        // Ensure all bodies were fully read; return the number of listed objects
        return std::to_string(n);
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