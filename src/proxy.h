#ifndef __PROXY_H__
#define __PROXY_H__

#include <thread>
#include "solver.h"

namespace yutovo_service
{

class Logger;
class ServiceContext;
class Config;

//Service proxy creates a solver and redirects messages
class Proxy
{
public:
    Proxy(ServiceContext* _service_context, Config* _config);
    ~Proxy();

private:
    void MessageLoop();

    void SendError(const ErrorCode error_code, zmq::socket_t& socket);
    void SendOk(zmq::socket_t& socket);
    void SendReply(const rapidjson::Document& json, zmq::socket_t& socket);

public:
    int idle_time = 0; //in seconds

private:
    std::thread message_loop;

    ServiceContext* service_context;
    Config* config;

    Logger* logger;

    bool exit = false;
};

typedef std::shared_ptr<Proxy> ProxyPtr;

}

#endif
