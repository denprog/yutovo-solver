#include <zmq.hpp>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "proxy.h"
#include "logger.h"
#include "service_context.h"

using namespace yutovo_service;

int main(int argc, char *argv[])
{
    Logger* logger = Logger::GetInstance(std::string(std::getenv("YUTOVO_DEPLOY")) + "/log", "solver", true, true);
    logger->Info("Yutovo service start");

    ServiceContext service_context;

    zmq::context_t context(1);

    zmq::socket_t frontend(context, ZMQ_ROUTER);
    zmq::socket_t proxy(context, ZMQ_DEALER);

    frontend.bind("tcp://*:8010");
    proxy.bind("tcp://*:8011");

    proxy.setsockopt(ZMQ_SNDTIMEO, 1000);

    zmq::pollitem_t items[] = 
        {
            {
                (void*)frontend, 0, ZMQ_POLLIN, 0
            },
            {
                (void*)proxy, 0, ZMQ_POLLIN, 0
            }
        };

    std::vector<ProxyPtr> proxies;

    while (!service_context.exit)
    {
        zmq::message_t message;
        int more;
        size_t more_size = sizeof(more);

        zmq::poll(items, 2, -1);

        if (items[0].revents & ZMQ_POLLIN)
        {
            while (true)
            {
                frontend.recv(&message);
                frontend.getsockopt(ZMQ_RCVMORE, &more, &more_size);
                if (proxy.send(message, more ? ZMQ_SNDMORE : 0) == 0)
                {
                    //add new proxy
                    proxies.emplace_back(new Proxy(&service_context));
                    if (proxy.send(message, more ? ZMQ_SNDMORE : 0) == 0)
                    {
                        logger->Error("Error sending message");
                    }
                }
                if (!more)
                    break;
            }
        }

        if (items[1].revents & ZMQ_POLLIN)
        {
            while (true)
            {
                proxy.recv(&message);
                proxy.getsockopt(ZMQ_RCVMORE, &more, &more_size);
                frontend.send(message, more ? ZMQ_SNDMORE : 0);
                if (!more)
                    break;
            }
        }
    }

    logger->Info("Yutovo service end");
    return 0;   
}
