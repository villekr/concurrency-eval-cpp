#include "../include/lambda_handler.h"
#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/utils/logging/LogLevel.h>
#include <aws/lambda-runtime/runtime.h>
#include <aws/s3/S3Client.h>

using namespace aws::lambda_runtime;
using namespace Aws::Utils::Logging;
using namespace Aws;

int main()
{
    SDKOptions options;
    options.loggingOptions.logLevel = LogLevel::Info;
    InitAPI(options);
    {
        auto handler_fn = [](invocation_request const& req) { return lambda_handler(req); };
        run_handler(handler_fn);
    }
    ShutdownAPI(options);
    return 0;
}
