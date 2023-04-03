#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <string>

namespace yutovo_service
{

class Logger;

class Config
{
public:
    Config(Logger* _logger);

    bool Read();

public:
    int threads_count = 2;
    int proxy_idle_timeout = 10; //in seconds
    int solver_idle_timeout = 20;

private:
    const std::string file_name = "service.ini";
    Logger* logger;
};

}

#endif
