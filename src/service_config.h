#ifndef __SERVICE_CONFIG_H__
#define __SERVICE_CONFIG_H__

#include <string>
#include <yutovo_logger/logger.h>

namespace yutovo_solver
{

class ServiceConfig
{
public:
    ServiceConfig(yutovo::Logger* _logger);

    bool Read();

public:
    unsigned short port = 8010;
    int threads_count = 2;
    bool wss = false;
    int proxy_idle_timeout = 10; //in seconds
    int solver_idle_timeout = 20;
    uint64_t max_time = 0; //calculation max time in milliseconds

private:
    const std::string file_name = "yutovo_solver.ini";
    yutovo::Logger* logger;
};

}

#endif
