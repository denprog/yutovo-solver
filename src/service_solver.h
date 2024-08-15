#ifndef __SERVICE_SOLVER_H__
#define __SERVICE_SOLVER_H__

#include <map>
#include <memory>
#include <mutex>
#include <yutovo_logger/logger.h>
#include <yutovo_calculator/parser.h>
#include "rapidjson/document.h"
#include "types.h"

namespace yutovo_solver
{

class ServiceConfig;

using namespace yutovo_calculator;
using namespace yutovo;

struct SolverLocale
{
    yutovo_calculator::Language language = Language::English;
};

class Solver
{
public:
    Solver(const std::string& _guid, const yutovo_calculator::Language _language);

    virtual void Solve(const rapidjson::Document& request, rapidjson::Document& reply) = 0;
    virtual void BreakSolving(const rapidjson::Document& request, rapidjson::Document& reply) = 0;
    virtual void RemoveIdentifier(const rapidjson::Document& request, rapidjson::Document& reply) = 0;
    virtual void ListIdentifiers(const rapidjson::Document& request, rapidjson::Document& reply) = 0;
    virtual bool SetLocale(const rapidjson::Document& request, rapidjson::Document& reply) = 0;
    virtual void SetMaxTime(const uint64_t max_time) = 0;

protected:
    void ReplyOk(rapidjson::Document& reply);
    void ReplyError(const ErrorCode error_code, rapidjson::Document& reply);
    void ReplyError(const yutovo_calculator::ParserException& ex, rapidjson::Document& reply);

    void AddUnit(rapidjson::Document& reply, const Unit& unit);
    void AddCastUnits(rapidjson::Document& reply, const std::vector<Unit>& cast_units);
    void AddDependencies(rapidjson::Document& reply, const std::vector<std::u32string>* dependencies);

    bool GetElementId(const rapidjson::Document& request, ElementId& id);
    bool GetTimestamp(const rapidjson::Document& request, uint64_t& time_stamp);

    rapidjson::Value ElementIdToValue(rapidjson::Document& reply, const ElementId& id);

    bool GetUnit(const rapidjson::Document& request, Unit& unit);

public:
    time_t idle_time = time(nullptr);

    std::string guid;
    SolverLocale locale;
};

typedef std::shared_ptr<Solver> SolverPtr;

class CalculatorSolver : public Solver
{
public:
    CalculatorSolver(const std::string& _guid, const yutovo_calculator::Language _language, uint64_t _max_time);
    ~CalculatorSolver();

    virtual void Solve(const rapidjson::Document& request, rapidjson::Document& reply);
    virtual void BreakSolving(const rapidjson::Document& request, rapidjson::Document& reply);
    virtual void RemoveIdentifier(const rapidjson::Document& request, rapidjson::Document& reply);
    virtual void ListIdentifiers(const rapidjson::Document& request, rapidjson::Document& reply);
    virtual bool SetLocale(const rapidjson::Document& request, rapidjson::Document& reply);
    virtual void SetMaxTime(const uint64_t max_time);

private:
    void SolveReal(const rapidjson::Document& request, rapidjson::Document& reply, std::vector<std::u32string>* dependencies);
    void SolveInteger(const rapidjson::Document& request, rapidjson::Document& reply, std::vector<std::u32string>* dependencies);
    void SolveRational(const rapidjson::Document& request, rapidjson::Document& reply, std::vector<std::u32string>* dependencies);
    void SolveComplex(const rapidjson::Document& request, rapidjson::Document& reply, std::vector<std::u32string>* dependencies);

    void AddReal(rapidjson::Document& reply, rapidjson::Value& obj, const Real& value, const int exponent_size, const int precision);

private:
    std::mutex parsers_lock;
    yutovo_calculator::Parser<yutovo_calculator::Real> real_parser;
    yutovo_calculator::Parser<yutovo_calculator::Integer> integer_parser;
    yutovo_calculator::Parser<yutovo_calculator::Rational> rational_parser;
    yutovo_calculator::Parser<yutovo_calculator::Complex> complex_parser;

    yutovo_calculator::ParserContext parser_context;

    std::mutex solving_id_lock;
    ElementId solving_id;
    uint64_t solving_time_stamp = 0;

    std::mutex break_lock;
    std::map<ElementId, int64_t> break_solvings;

    bool just_started = true;

    Logger* logger = nullptr;
};

class PythonSolver : public Solver
{
public:
    PythonSolver(const std::string& _guid, const yutovo_calculator::Language _language, uint64_t _max_time);
    ~PythonSolver();

    virtual void Solve(const rapidjson::Document& request, rapidjson::Document& reply);
    virtual void BreakSolving(const rapidjson::Document& request, rapidjson::Document& reply);
    virtual void RemoveIdentifier(const rapidjson::Document& request, rapidjson::Document& reply);
    virtual void ListIdentifiers(const rapidjson::Document& request, rapidjson::Document& reply);
    virtual bool SetLocale(const rapidjson::Document& request, rapidjson::Document& reply);
    virtual void SetMaxTime(const uint64_t max_time);

private:
    Logger* logger = nullptr;
};

class Solvers
{
public:
    Solvers(ServiceConfig* _config);

    SolverPtr GetSolver(const std::string& guid, const int code_id, SolverType solver_type);
    void SetLocale(const std::string& guid, const yutovo_calculator::Language language, 
        const rapidjson::Document& request, rapidjson::Document& reply);
    void SetMaxTime(const uint64_t max_time);
    void RemoveTimeouted();

private:
    std::mutex solvers_mutex;
    std::map<std::string, std::map<int, SolverPtr>> solvers; //by guid and by code_id
    std::map<std::string, SolverLocale> solvers_locales; //by guid

    ServiceConfig* config;
};

}

#endif
