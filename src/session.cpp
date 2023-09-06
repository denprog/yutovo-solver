#include "session.h"
#include "solver.h"
#include "service_context.h"

namespace yutovo_service
{

//Session

int Session::sessions_count = 0;

Session::Session(ServiceContext* _service_context, Logger* _logger) :
    service_context(_service_context),
    logger(_logger)
{
    logger->Info("Sessions count: {}", ++sessions_count);
}

Session::~Session()
{
    logger->Info("Sessions count: {}", --sessions_count);
}

void Session::Parse(const std::string& json, std::string& reply)
{
    rapidjson::Document request_json;
    request_json.Parse<0>(json.c_str());
    if (request_json.HasParseError())
    {
        MakeError(ErrorCode::JSON_ERROR, reply);
        return;
    }

    if (!request_json.HasMember("command") || !request_json["command"].IsString())
    {
        MakeError(ErrorCode::NO_FIELD_ERROR, reply);
        return;
    }

    std::string command = request_json["command"].GetString();
    SolverPtr solver;

    if (command == "EXIT")
    {
        service_context->exit = true;
        MakeOk(reply);
        return;
    }
    else if (command == "SOLVE_CODE" || command == "REMOVE_IDENTIFIER" || command == "LIST_IDENTIFIERS")
    {
        if (!request_json.HasMember("guid") || !request_json["guid"].IsString())
        {
            MakeError(ErrorCode::NO_FIELD_ERROR, reply);
            return;
        }
        std::string guid = request_json["guid"].GetString();

        if (!request_json.HasMember("code_id") || !request_json["code_id"].IsInt())
        {
            MakeError(ErrorCode::NO_FIELD_ERROR, reply);
            return;
        }
        int code_id = request_json["code_id"].GetInt();

        if (!request_json.HasMember("solver_type") || !request_json["solver_type"].IsInt())
        {
            MakeError(ErrorCode::NO_FIELD_ERROR, reply);
            return;
        }
        SolverType solver_type = (SolverType)request_json["solver_type"].GetInt();

        std::string solver_id = guid + "-" + std::to_string(code_id);
        solver = service_context->solvers.GetSolver(solver_id, solver_type);
        if (!solver)
        {
            MakeError(ErrorCode::SOLVER_ERROR, reply);
            return;
        }
    }
    else
    {
        MakeError(ErrorCode::UNKNOWN_COMMAND, reply);
        return;
    }

    rapidjson::Document response_json;
    response_json.SetObject();
    if (command == "SOLVE_CODE")
    {
        solver->Solve(request_json, response_json);
        MakeReply(response_json, reply);
        return;
    }

    if (command == "REMOVE_IDENTIFIER")
    {
        solver->RemoveIdentifier(request_json, response_json);
        MakeReply(response_json, reply);
        return;
    }

    if (command == "LIST_IDENTIFIERS")
    {
        solver->ListIdentifiers(request_json, response_json);
        MakeReply(response_json, reply);
        return;
    }
}

void Session::MakeError(const ErrorCode error_code, std::string& reply)
{
    rapidjson::Document response_json;
    auto& alloc = response_json.GetAllocator();
    response_json.SetObject();
    response_json.AddMember("error", (int)error_code, alloc);
    MakeReply(response_json, reply);
}

void Session::MakeOk(std::string& reply)
{
    rapidjson::Document response_json;
    auto& alloc = response_json.GetAllocator();
    response_json.SetObject();
    response_json.AddMember("result", (int)ErrorCode::OK, alloc);
    MakeReply(response_json, reply);
}

void Session::MakeReply(const rapidjson::Document& json, std::string& reply)
{
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    json.Accept(writer);
    reply = buffer.GetString();
}

}
