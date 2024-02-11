#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <string>
#include <yutovo_logger/logger.h>

namespace yutovo_service
{

using namespace yutovo;

class Config
{
public:
    Config(Logger* _logger);

    bool Read();

public:
    unsigned short port = 8010;
    int threads_count = 2;
    bool wss = false;
    int proxy_idle_timeout = 10; //in seconds
    int solver_idle_timeout = 20;

private:
    const std::string file_name = "service.ini";
    Logger* logger;
};

}

#endif
