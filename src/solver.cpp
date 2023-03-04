#include "solver.h"
#include "logger.h"
#include "config.h"

namespace yutovo_service
{

//Solver

Solver::Solver(const std::string& _guid) :
    guid(_guid)
{
}

void Solver::ReplyError(const ErrorCode error_code, rapidjson::Document& reply)
{
    rapidjson::Value error;
    error.SetObject();
    auto& alloc = reply.GetAllocator();
    error.AddMember("error_code", (int)error_code, alloc);
    reply.AddMember("error", error, alloc);
}

void Solver::ReplyError(const yutovo_calculator::ParserException ex, rapidjson::Document& reply)
{
    rapidjson::Value error;
    error.SetObject();
    auto& alloc = reply.GetAllocator();
    error.AddMember("error_code", (int)ErrorCode::PARSER_ERROR, alloc);
    error.AddMember("parser_error_code", ex.id, alloc);
    error.AddMember("pos", ex.pos, alloc);
    error.AddMember("line", ex.line, alloc);
    reply.AddMember("error", error, alloc);
}

//CalculatorSolver

CalculatorSolver::CalculatorSolver(const std::string& _guid) :
    Solver(_guid),
    real_parser(0),
    integer_parser(0),
    rational_parser(0),
    logger(Logger::GetInstance(std::string(std::getenv("YUTOVO_DEPLOY")) + "/log", "calculator_solver", true, true))
{
    logger->Info("Calculator Solver started: {}", guid);
}

CalculatorSolver::~CalculatorSolver()
{
    logger->Info("Calculator Solver finished: {}", guid);
}

void CalculatorSolver::Solve(const rapidjson::Document& request, rapidjson::Document& reply)
{
    idle_time = time(nullptr);

    reply.SetObject();
    if (just_started)
    {
        just_started = false;
        ReplyError(ErrorCode::SOLVER_RESTARTED_ERROR, reply);
        return;
    }

    if (!request.HasMember("result_type") || !request["result_type"].IsInt())
    {
        logger->Error("result_type error");
        ReplyError(ErrorCode::NO_FIELD_ERROR, reply);
        return;
    }

    if (!request.HasMember("expression") || !request["expression"].IsString())
    {
        logger->Error("expression error");
        ReplyError(ErrorCode::NO_FIELD_ERROR, reply);
        return;
    }

    std::string expression = request["expression"].GetString();
    ResultType result_type = (ResultType)request["result_type"].GetInt();

    auto& alloc = reply.GetAllocator();

    switch (result_type)
    {
    case ResultType::REAL:
        {
            int precision = 3;
            if (request.HasMember("precision") && request["precision"].IsInt())
                precision = request["precision"].GetInt();

            int angle_measure = 1;
            if (request.HasMember("angle_measure") && request["angle_measure"].IsInt())
                angle_measure = request["angle_measure"].GetInt();

            int accuracy_size = 3;
            if (request.HasMember("accuracy_size") && request["accuracy_size"].IsInt())
                accuracy_size = request["accuracy_size"].GetInt();

            int exponent_size = 3;
            if (request.HasMember("exponent_size") && request["exponent_size"].IsInt())
                exponent_size = request["exponent_size"].GetInt();
            
            yutovo_calculator::Real res;

            try
            {
                real_parser.SetPrecision(precision);
                res = real_parser.Parse(expression);
            }
            catch (yutovo_calculator::ParserException ex)
            {
                ReplyError(ex, reply);
                break;
            }

            bool mantissa_sign;
            std::string mantissa;
            bool exponent_sign;
            std::string exponent;
            res.ToString(accuracy_size, exponent_size, mantissa_sign, mantissa, exponent_sign, exponent);
            if (mantissa_sign)
                mantissa.insert(mantissa.begin(), '-');
            rapidjson::Value m(rapidjson::kStringType);
            m.SetString(mantissa.c_str(), mantissa.size(), alloc);
            reply.AddMember("mantissa", m, alloc);
            if (exponent_sign)
                exponent.insert(exponent.begin(), '-');
            if (exponent != "")
            {
                rapidjson::Value e(rapidjson::kStringType);
                e.SetString(exponent.c_str(), exponent.size(), alloc);
                reply.AddMember("exponent", e, alloc);
            }
        }
        break;
    case ResultType::INTEGER:
        {
            int notation = 1;
            if (request.HasMember("notation") && request["notation"].IsInt())
                notation = request["notation"].GetInt();
            
            yutovo_calculator::Integer res;
            try
            {
                res = integer_parser.Parse(expression);
            }
            catch (yutovo_calculator::ParserException ex)
            {
                ReplyError(ex, reply);
                break;
            }

            std::string s = res.ToStdString();
            rapidjson::Value val(rapidjson::kStringType);
            val.SetString(s.c_str(), s.size(), alloc);
            reply.AddMember("value", val, alloc);
        }
        break;
    case ResultType::RATIONAL:
        {
            yutovo_calculator::Rational res;
            try
            {
                res = rational_parser.Parse(expression);
            }
            catch (yutovo_calculator::ParserException ex)
            {
                ReplyError(ex, reply);
                break;
            }

            std::string numerator = res.GetNumerator().ToStdString();
            std::string denomerator = res.GetDenomerator().ToStdString();
            rapidjson::Value n(rapidjson::kStringType);
            n.SetString(numerator.c_str(), numerator.size(), alloc);
            reply.AddMember("numerator", n, alloc);
            rapidjson::Value d(rapidjson::kStringType);
            d.SetString(denomerator.c_str(), denomerator.size(), alloc);
            reply.AddMember("denomerator", d, alloc);
        }
        break;
    }
}

void CalculatorSolver::RemoveIdentifier(const rapidjson::Document& request, rapidjson::Document& reply)
{
    idle_time = time(nullptr);

    if (!request.HasMember("expression") || !request["expression"].IsString())
    {
        logger->Error("expression error");
        ReplyError(ErrorCode::NO_FIELD_ERROR, reply);
        return;
    }
    std::string identifier = request["expression"].GetString();

    try
    {
        real_parser.RemoveIdentifier(identifier);
        integer_parser.RemoveIdentifier(identifier);
        rational_parser.RemoveIdentifier(identifier);
    }
    catch (yutovo_calculator::ParserException ex)
    {
        ReplyError(ex, reply);
        return;
    }

    ReplyError(ErrorCode::OK, reply);
}

//PythonSolver

PythonSolver::PythonSolver(const std::string& _guid) :
    Solver(_guid),
    logger(Logger::GetInstance(std::string(std::getenv("YUTOVO_DEPLOY")) + "/log", "python_solver", true, true))
{
    logger->Info("Python Solver started: {}", guid);
}

PythonSolver::~PythonSolver()
{
    logger->Info("Python Solver finished: {}", guid);
}

void PythonSolver::Solve(const rapidjson::Document& request, rapidjson::Document& reply)
{
    idle_time = time(nullptr);
}

void PythonSolver::RemoveIdentifier(const rapidjson::Document& request, rapidjson::Document& reply)
{
    idle_time = time(nullptr);
}

//Solvers

Solvers::Solvers(Config* _config) :
    config(_config)
{
}

SolverPtr Solvers::GetSolver(const std::string& solver_id, SolverType solver_type)
{
    std::lock_guard<std::mutex> lock(solvers_mutex);
    auto it = solvers.find(solver_id);
    if (it != solvers.end())
        return it->second;
    
    switch (solver_type)
    {
    case SolverType::CALCULATOR:
        {
            SolverPtr solver(new CalculatorSolver(solver_id));
            solvers[solver_id] = solver;
            return solver;
        }
    case SolverType::PYTHON:
        break;
    }

    return nullptr;
}

void Solvers::RemoveTimeouted()
{
    std::lock_guard<std::mutex> lock(solvers_mutex);
    for (auto it = solvers.begin(); it != solvers.end();)
    {
        if (time(nullptr) - it->second->idle_time > config->solver_idle_timeout)
        {
            solvers.erase(it++);
            continue;
        }
        ++it;
    }
}

}
