#ifndef __PROXY_H__
#define __PROXY_H__

#include <thread>
#include "solver.h"

namespace yutovo_service
{

class Logger;

//Service proxy creates a solver and redirects messages
class Proxy
{
public:
    Proxy(Solvers& _solvers);
    ~Proxy();

private:
    void MessageLoop();

    void SendError(const ErrorCode error_code, zmq::socket_t& socket);
    void SendReply(const rapidjson::Document& json, zmq::socket_t& socket);

private:
    std::thread message_loop;

    Solvers& solvers;

    Logger* logger;

    bool exit = false;
};

typedef std::shared_ptr<Proxy> ProxyPtr;

}

#endif
