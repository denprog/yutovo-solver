#ifndef __SESSION_H__
#define __SESSION_H__

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "types.h"
#include <yutovo_logger/logger.h>

namespace yutovo_service
{

class ServiceContext;
class Config;

using namespace yutovo;

class Session
{
public:
    Session(ServiceContext* _service_context, Logger* _logger);

    void Parse(const std::string& json, std::string& reply);

private:
    void MakeError(const ErrorCode error_code, std::string& reply);
    void MakeOk(std::string& reply);
    void MakeReply(const rapidjson::Document& json, std::string& reply);

private:
    ServiceContext* service_context;
    Logger* logger;
};

}

#endif
