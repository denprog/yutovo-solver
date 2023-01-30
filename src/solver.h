#ifndef __SOLVERS_H__
#define __SOLVERS_H__

#include <zmq.hpp>
#include <map>
#include <memory>
#include <yutovo_calculator/parser.h>
#include "rapidjson/document.h"

namespace yutovo_service
{

class Logger;

enum class SolverType
{
    NONE = 0,
    CALCULATOR = 1,
    PYTHON = 2
};

//Returned result type
enum class ResultType
{
	NONE = 0, 
	REAL, 
	INTEGER, 
	RATIONAL, 
	COMPLEX, 
	AUTO
};

enum class ErrorCode
{
    NONE = 0,
    JSON_ERROR, //json was not parsed
    NO_FIELD_ERROR, //there is no field requeried
    SOLVER_ERROR, //solver was not created or executed
    EXPRESSION_ERROR, //expression was not parsed
    SOLVER_TIMEOUT_ERROR, 
    SOLVER_RESTARTED_ERROR //all connected code blocks need to be reevaluated
};

class Solver
{
public:
    virtual void Solve(const rapidjson::Document& request, rapidjson::Document& reply) = 0;

protected:
    void ReplyError(const ErrorCode error_code, rapidjson::Document& reply);
    void ReplyError(const yutovo_calculator::ParserException ex, rapidjson::Document& reply);
};

typedef std::shared_ptr<Solver> SolverPtr;

class CalculatorSolver : public Solver
{
public:
    CalculatorSolver();

    virtual void Solve(const rapidjson::Document& request, rapidjson::Document& reply);

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
    virtual void Solve(const rapidjson::Document& request, rapidjson::Document& reply);
};

class Solvers
{
public:
    Solvers();

    SolverPtr GetSolver(const std::string& solver_id, SolverType solver_type);

private:
    std::map<std::string, SolverPtr> solvers;
};

}

#endif
