#ifndef LAMBDA_HANDLER_H
#define LAMBDA_HANDLER_H

#include <aws/lambda-runtime/runtime.h>

using namespace std;
using namespace aws::lambda_runtime;

invocation_response lambda_handler(invocation_request const& req);

#endif // LAMBDA_HANDLER_H
