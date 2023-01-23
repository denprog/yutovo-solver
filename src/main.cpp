#include <zmq.hpp>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "calc_solver.h"
#include "logger.h"

using namespace yutovo_service;

int main(int argc, char *argv[])
{
    Logger* logger = Logger::GetInstance("programs/Math/bin/", "calc_solver", true, true);
    logger->Info("Yutovo service start");

    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_REP);
    socket.bind("tcp://*:8010");

    while (true)
    {
        zmq::message_t request;
        socket.recv(&request);

        std::string json = std::string((const char*)request.data(), request.size());
        logger->Info("Request received:\n{}", json);

        rapidjson::Document doc;
        doc.Parse<0>(json.c_str());
        if (doc.HasParseError())
        {
            logger->Error("Error parsing request");
            continue;
        }
        if (!doc.HasMember("solver_type") || !doc["solver_type"].IsInt())
        {
            logger->Error("Error of solver_type");
            continue;
        }

        rapidjson::Document reply_json;
        switch (doc["solver_type"].GetInt())
        {
        case 1:
            {
                CalcSolver solver;
                solver.Solve(doc, reply_json);
            }
            break;
        default:
            reply_json.SetObject();
            auto& alloc = reply_json.GetAllocator();
            reply_json.AddMember("error", "solver_type error", alloc);
            break;
        }

        rapidjson::StringBuffer buffer;
        //rapidjson::Writer<rapidjson::StringBuffer, rapidjson::Document::EncodingType, rapidjson::ASCII<>> writer(buffer);
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        reply_json.Accept(writer);
        json = buffer.GetString();

        zmq::message_t reply(json.size());
        std::memcpy(reply.data(), json.data(), json.size());
        socket.send(reply);
        logger->Info("Response sent:\n{}", json);
    }

    logger->Info("Yutovo service end");
    return 0;   
}

// int main(int argc, char *argv[])
// {
    // asio::io_service io_service;
    // azmq::rep_socket socket(io_service);
    // socket.bind("tcp://*:8010");

    // std::array<char, 256> buf;
    // bool exit = false;
    // while (!exit)
    // {
    //     azmq::message message;
    //     auto size = socket.receive(message);
    //     while (message.more())
    //         size = socket.receive(message, ZMQ_RCVMORE);
        
    //     azmq::message reply;
    //     socket.send(reply);

    //     //auto size = socket.receive(asio::buffer(buf));
    //     // socket.async_receive(
    //     //     [&](const boost::system::error_code& ec, azmq::message& message, size_t)
    //     //     {
    //     //         azmq::message reply_message;
    //     //         socket.async_send(reply_message,
    //     //             [&](const boost::system::error_code& ec, size_t)
    //     //             {
    //     //             });
    //     //     }
    //     // );
    // }
    // // azmq::sub_socket subscriber(ios);
    // // subscriber.connect("tcp://192.168.55.112:5556");
    // // subscriber.connect("tcp://192.168.55.201:7721");
    // // subscriber.set_option(azmq::socket::subscribe("NASDAQ"));

    // // azmq::pub_socket publisher(ios);
    // // publisher.bind("ipc://nasdaq-feed");

    // // std::array<char, 256> buf;
    // // for (;;) {
    // //     auto size = subscriber.receive(asio::buffer(buf));
    // //     publisher.send(asio::buffer(buf));
    // // }
    // return 0;
//}
