#include "proxy.h"
#include <zmq.hpp>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "logger.h"
#include "service_context.h"

namespace yutovo_service
{

//Proxy

Proxy::Proxy(ServiceContext* _service_context) :
    message_loop(std::thread(&Proxy::MessageLoop, this)),
    service_context(_service_context),
    logger(Logger::GetInstance(std::string(std::getenv("YUTOVO_DEPLOY")) + "/log", "solver", true, true))
{
}

Proxy::~Proxy()
{
    exit = true;
    message_loop.join();
}

void Proxy::MessageLoop()
{
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_REP);
    socket.setsockopt(ZMQ_RCVTIMEO, 1000);
    socket.connect("tcp://localhost:8011");

    while (!exit)
    {
        zmq::message_t request;
        if (socket.recv(&request) == 0)
            continue;
        
        //get GUID and code_id from the message
        std::string json = std::string((const char*)request.data(), request.size());
        logger->Info("Request received:\n{}", json);
        rapidjson::Document request_json;
        request_json.Parse<0>(json.c_str());
        if (request_json.HasParseError())
        {
            SendError(ErrorCode::JSON_ERROR, socket);
            continue;
        }

        if (!request_json.HasMember("command") || !request_json["command"].IsString())
        {
            SendError(ErrorCode::NO_FIELD_ERROR, socket);
            continue;
        }

        std::string command = request_json["command"].GetString();
        SolverPtr solver;

        if (command == "EXIT")
        {
            service_context->exit = true;
            SendOk(socket);
            continue;
        }
        else if (command == "SOLVE_CODE" || command == "REMOVE_IDENTIFIER")
        {
            if (!request_json.HasMember("guid") || !request_json["guid"].IsString())
            {
                SendError(ErrorCode::NO_FIELD_ERROR, socket);
                continue;
            }
            std::string guid = request_json["guid"].GetString();

            if (!request_json.HasMember("code_id") || !request_json["code_id"].IsInt())
            {
                SendError(ErrorCode::NO_FIELD_ERROR, socket);
                continue;
            }
            int code_id = request_json["code_id"].GetInt();

            if (!request_json.HasMember("solver_type") || !request_json["solver_type"].IsInt())
            {
                SendError(ErrorCode::NO_FIELD_ERROR, socket);
                continue;
            }
            SolverType solver_type = (SolverType)request_json["solver_type"].GetInt();

            std::string solver_id = guid + "-" + std::to_string(code_id);
            solver = service_context->solvers.GetSolver(solver_id, solver_type);
            if (!solver)
            {
                SendError(ErrorCode::SOLVER_ERROR, socket);
                continue;
            }
        }
        else
        {
            SendError(ErrorCode::UNKNOWN_COMMAND, socket);
            continue;
        }

        rapidjson::Document response_json;
        response_json.SetObject();
        if (command == "SOLVE_CODE")
        {
            solver->Solve(request_json, response_json);
            SendReply(response_json, socket);
            continue;
        }

        if (command == "REMOVE_IDENTIFIER")
        {
            solver->RemoveIdentifier(request_json, response_json);
            SendReply(response_json, socket);
            continue;
        }
    }
}

void Proxy::SendError(const ErrorCode error_code, zmq::socket_t& socket)
{
    rapidjson::Document response_json;
    auto& alloc = response_json.GetAllocator();
    rapidjson::Value error;
    error.SetObject();
    response_json.AddMember("error", (int)error_code, alloc);
    SendReply(response_json, socket);
}

void Proxy::SendOk(zmq::socket_t& socket)
{
    rapidjson::Document response_json;
    auto& alloc = response_json.GetAllocator();
    rapidjson::Value result;
    result.SetObject();
    response_json.AddMember("result", (int)ErrorCode::OK, alloc);
    SendReply(response_json, socket);
}

void Proxy::SendReply(const rapidjson::Document& json, zmq::socket_t& socket)
{
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    json.Accept(writer);
    std::string json_str = buffer.GetString();

    zmq::message_t reply(json_str.size());
    std::memcpy(reply.data(), json_str.data(), json_str.size());
    if (socket.send(reply) == 0)
        logger->Error("Error sending message from proxy");
    else
        logger->Info("Reply sent:\n{}", json_str);
}

}
