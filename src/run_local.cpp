#include "../include/lambda_handler.h"
#include <aws/core/Aws.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/lambda-runtime/runtime.h>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace std;
using namespace Aws;
using namespace Aws::Utils::Logging;
using namespace Aws::Utils::Json;
using namespace aws::lambda_runtime;

string get_from_env(const string& env_variable)
{
    const char* value = getenv(env_variable.c_str());
    if (value != nullptr) {
        return {value};
    }
    throw runtime_error(env_variable + " environment variable is not defined");
}

void invoke_lambda_handler()
{
    string const s3_bucket_name = get_from_env("S3_BUCKET_NAME");
    string const folder = get_from_env("FOLDER");
    string const find = get_from_env("FIND");

    Aws::Utils::Json::JsonValue event_payload;
    event_payload.WithString("s3_bucket_name", s3_bucket_name);
    event_payload.WithString("folder", folder);

    invocation_request request;
    request.payload = event_payload.View().WriteCompact();

    SDKOptions options;
    options.loggingOptions.logLevel = LogLevel::Info;
    InitAPI(options);
    {
        auto response = lambda_handler(request);
        cout << "Response: " << response.get_payload() << endl;

        event_payload.WithString("find", find);
        string const event_payload_str = event_payload.View().WriteCompact();

        request.payload = event_payload_str;
        auto response_with_find = lambda_handler(request);
        cout << "Response: " << response_with_find.get_payload() << endl;
    }
    ShutdownAPI(options);
}

int main()
{
    invoke_lambda_handler();
    return 0;
}
