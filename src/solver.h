#ifndef __SOLVERS_H__
#define __SOLVERS_H__

#include <map>
#include <memory>
#include <mutex>
#include <yutovo_logger/logger.h>
#include <yutovo_calculator/parser.h>
#include "rapidjson/document.h"
#include "types.h"

namespace yutovo_service
{

class Config;

using namespace yutovo_calculator;
using namespace yutovo;

class Solver
{
public:
    Solver(const std::string& _guid);

    virtual void Solve(const rapidjson::Document& request, rapidjson::Document& reply) = 0;
    virtual void RemoveIdentifier(const rapidjson::Document& request, rapidjson::Document& reply) = 0;
    virtual void ListIdentifiers(const rapidjson::Document& request, rapidjson::Document& reply) = 0;

protected:
    void ReplyError(const ErrorCode error_code, rapidjson::Document& reply);
    void ReplyError(const yutovo_calculator::ParserException& ex, rapidjson::Document& reply);

    void AddUnit(rapidjson::Document& reply, const Unit& unit);
    void AddCastUnits(rapidjson::Document& reply, const std::vector<Unit>& cast_units);
    void AddDependencies(rapidjson::Document& reply, const std::vector<std::u32string>& dependencies);

    bool GetElementId(const rapidjson::Document& request, ElementId& id);
    rapidjson::Value ElementIdToValue(rapidjson::Document& reply, const ElementId& id);

    bool GetUnit(const rapidjson::Document& request, Unit& unit);

public:
    time_t idle_time = time(nullptr);

protected:
    std::string guid;
};

typedef std::shared_ptr<Solver> SolverPtr;

class CalculatorSolver : public Solver
{
public:
    CalculatorSolver(const std::string& _guid);
    ~CalculatorSolver();

    virtual void Solve(const rapidjson::Document& request, rapidjson::Document& reply);
    virtual void RemoveIdentifier(const rapidjson::Document& request, rapidjson::Document& reply);
    virtual void ListIdentifiers(const rapidjson::Document& request, rapidjson::Document& reply);

private:
    void SolveReal(const rapidjson::Document& request, rapidjson::Document& reply, std::vector<std::u32string>& dependencies);
    void SolveInteger(const rapidjson::Document& request, rapidjson::Document& reply, std::vector<std::u32string>& dependencies);
    void SolveRational(const rapidjson::Document& request, rapidjson::Document& reply, std::vector<std::u32string>& dependencies);

private:
    yutovo_calculator::Parser<yutovo_calculator::Real> real_parser;
    yutovo_calculator::Parser<yutovo_calculator::Integer> integer_parser;
    yutovo_calculator::Parser<yutovo_calculator::Rational> rational_parser;

    bool just_started = true;

    Logger* logger;
};

class PythonSolver : public Solver
{
public:
    PythonSolver(const std::string& _guid);
    ~PythonSolver();

    virtual void Solve(const rapidjson::Document& request, rapidjson::Document& reply);
    virtual void RemoveIdentifier(const rapidjson::Document& request, rapidjson::Document& reply);
    virtual void ListIdentifiers(const rapidjson::Document& request, rapidjson::Document& reply);

private:
    Logger* logger;
};

class Solvers
{
public:
    Solvers(Config* _config);

    SolverPtr GetSolver(const std::string& solver_id, SolverType solver_type);
    void RemoveTimeouted();

private:
    std::mutex solvers_mutex;
    std::map<std::string, SolverPtr> solvers;
    Config* config;
};

}

#endif
