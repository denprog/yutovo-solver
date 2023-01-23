#ifndef __CALC_SOLVER_H__
#define __CALC_SOLVER_H__

#include <yutovo_calculator/parser.h>
#include "logger.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

namespace yutovo_service
{

template<class Number>
class CalcParser
{
public:
};

class CalcSolver
{
public:
    CalcSolver();
    ~CalcSolver();

    bool Solve(const rapidjson::Document& request, rapidjson::Document& reply);

private:
    Logger* logger;
};

}

#endif
