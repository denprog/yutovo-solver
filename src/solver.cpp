#include "solver.h"
#include "logger.h"
#include "config.h"
#include <yutovo_calculator/integer.h>

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
    error.AddMember("id", ElementIdToValue(reply, ex.id), alloc);
    error.AddMember("error_code", (int)ErrorCode::PARSER_ERROR, alloc);
    error.AddMember("parser_error_code", ex.ex_id, alloc);
    error.AddMember("pos", ex.pos, alloc);
    error.AddMember("line", ex.line, alloc);
    rapidjson::Value s((boost::locale::conv::utf_to_utf<char>(ex.description)).c_str(), alloc);
    error.AddMember("description", s, alloc);
    reply.AddMember("error", error, alloc);
}

void Solver::AddUnit(rapidjson::Document& reply, const Unit& unit)
{
    if (unit.IsEmpty())
        return;

    auto& alloc = reply.GetAllocator();
    rapidjson::Value _unit;
    _unit.SetObject();
    rapidjson::Value d(rapidjson::kArrayType);
    for (auto& u : unit.unit)
    {
        rapidjson::Value _u;
        _u.SetObject();
        rapidjson::Value s((boost::locale::conv::utf_to_utf<char>(u.first)).c_str(), alloc);
        _u.AddMember("name", s, alloc);
        if (u.second != 1)
            _u.AddMember("power", u.second, alloc);
        d.PushBack(_u, alloc);
    }
    if (unit.system != U"")
    {
        rapidjson::Value s((boost::locale::conv::utf_to_utf<char>(unit.system)).c_str(), alloc);
        _unit.AddMember("system", s, alloc);
    }
    _unit.AddMember("value", d, alloc);
    reply.AddMember("unit", _unit, alloc);
}

void Solver::AddCastUnits(rapidjson::Document& reply, const std::vector<Unit>& cast_units)
{
    if (cast_units.empty())
        return;
    
    //sort the cast units by those systems
    std::map<std::u32string, std::vector<Unit>> system_units;
    for (const Unit& unit : cast_units)
    {
        if (unit.system == U"")
            system_units[U"SI"].push_back(unit);
        else
            system_units[unit.system].push_back(unit);
    }

    //make json arrays
    auto& alloc = reply.GetAllocator();
    rapidjson::Value systems(rapidjson::kArrayType);
    for (auto& s : system_units)
    {
        rapidjson::Value system;
        system.SetObject();
        system.AddMember("system", rapidjson::Value((boost::locale::conv::utf_to_utf<char>(s.first)).c_str(), alloc), alloc);

        rapidjson::Value units(rapidjson::kArrayType);
        for (auto& unit : s.second)
        {
            rapidjson::Value unit_array(rapidjson::kArrayType);
            for (auto& u : unit.unit)
            {
                rapidjson::Value _u;
                _u.SetObject();
                rapidjson::Value s((boost::locale::conv::utf_to_utf<char>(u.first)).c_str(), alloc);
                _u.AddMember("name", s, alloc);
                if (u.second != 1)
                    _u.AddMember("power", u.second, alloc);
                unit_array.PushBack(_u, alloc);
            }
            units.PushBack(unit_array, alloc);
        }
        system.AddMember("units", units, alloc);

        systems.PushBack(system, alloc);
    }

    reply.AddMember("cast_units", systems, alloc);
}

void Solver::AddDependencies(rapidjson::Document& reply, const std::vector<std::u32string>& dependencies)
{
    if (dependencies.empty())
        return;
    
    auto& alloc = reply.GetAllocator();
    rapidjson::Value d(rapidjson::kArrayType);
    for (auto& str : dependencies)
    {
        rapidjson::Value s((boost::locale::conv::utf_to_utf<char>(str)).c_str(), alloc);
        d.PushBack(s, alloc);
    }
    reply.AddMember("dependencies", d, alloc);
}

bool Solver::GetElementId(const rapidjson::Document& request, ElementId& id)
{
    if (!request.HasMember("id") || !request["id"].IsArray())
        return false;
    rapidjson::GenericArray arr = request["id"].GetArray();
    id.clear();
    for (rapidjson::SizeType i = 0; i < arr.Size(); ++i)
    {
        if (!arr[i].IsInt())
            return false;
        id.push_back(arr[i].GetInt());
    }
    return true;
}

rapidjson::Value Solver::ElementIdToValue(rapidjson::Document& reply, const ElementId& id)
{
    auto& alloc = reply.GetAllocator();
    rapidjson::Value d(rapidjson::kArrayType);
    for (int i : id)
        d.PushBack(i, alloc);
    return d;
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

    if (!request.HasMember("result_type") || !request["result_type"].IsInt() || 
        !request.HasMember("expression") || !request["expression"].IsString())
    {
        logger->Error("result_type error");
        ReplyError(ErrorCode::NO_FIELD_ERROR, reply);
        return;
    }

    ResultType result_type = (ResultType)request["result_type"].GetInt();

    std::vector<std::u32string> dependencies;

    switch (result_type)
    {
    case ResultType::AUTO:
        {
            if (!request.HasMember("results_order") || !request["results_order"].IsArray())
            {
                logger->Error("results_order error");
                ReplyError(ErrorCode::NO_FIELD_ERROR, reply);
                return;
            }
            std::vector<ResultType> results_order;
            const rapidjson::Value& r = request["results_order"];
            for (rapidjson::SizeType i = 0; i < r.Size(); i++)
            {
                if (!r[i].IsInt())
                {
                    logger->Error("results_order error");
                    ReplyError(ErrorCode::FIELD_ERROR, reply);
                    return;
                }
                results_order.push_back((ResultType)r[i].GetInt());
            }

            //try all the parsers in the requered order until one of them parses
            rapidjson::Document error_reply;
            error_reply.CopyFrom(reply, error_reply.GetAllocator());
            for (size_t i = 0; i < results_order.size(); ++i)
            {
                ResultType t = results_order[i];
                try
                {
                    switch (t)
                    {
                    case ResultType::REAL:
                        SolveReal(request, reply, dependencies);
                        return;
                    case ResultType::INTEGER:
                        SolveInteger(request, reply, dependencies);
                        return;
                    case ResultType::RATIONAL:
                        SolveRational(request, reply, dependencies);
                        return;
                    }
                }
                catch (yutovo_calculator::ParserException& ex)
                {
                    if (i == 0)
                    {
                        ReplyError(ex, error_reply);
                        AddDependencies(error_reply, dependencies);
                    }
                }
                catch (ServiceException& ex)
                {
                    if (i == 0)
                    {
                        ReplyError(ex.error_code, error_reply);
                        AddDependencies(error_reply, dependencies);
                    }
                }
            }

            //none of the parsers has parsed
            reply.CopyFrom(error_reply, reply.GetAllocator());
        }
        break;
    case ResultType::REAL:
    case ResultType::INTEGER:
    case ResultType::RATIONAL:
        try
        {
            switch (result_type)
            {
            case ResultType::REAL:
                SolveReal(request, reply, dependencies);
                break;
            case ResultType::INTEGER:
                SolveInteger(request, reply, dependencies);
                break;
            case ResultType::RATIONAL:
                SolveRational(request, reply, dependencies);
                break;
            }
        }
        catch (yutovo_calculator::ParserException& ex)
        {
            ReplyError(ex, reply);
            AddDependencies(reply, dependencies);
        }
        catch (ServiceException& ex)
        {
            ReplyError(ex.error_code, reply);
            AddDependencies(reply, dependencies);
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

    ElementId id;
    if (!GetElementId(request, id))
        return;
    std::string identifier = request["expression"].GetString();

    try
    {
        real_parser.RemoveIdentifier(id, identifier);
        integer_parser.RemoveIdentifier(id, identifier);
        rational_parser.RemoveIdentifier(id, identifier);
    }
    catch (yutovo_calculator::ParserException ex)
    {
        ReplyError(ex, reply);
        return;
    }

    ReplyError(ErrorCode::OK, reply);
}

void CalculatorSolver::SolveReal(const rapidjson::Document& request, rapidjson::Document& reply, std::vector<std::u32string>& dependencies)
{
    ElementId id;
    if (!GetElementId(request, id))
    {
        logger->Error("id error");
        throw ServiceException{ErrorCode::NO_FIELD_ERROR};
    }

    std::string expression = request["expression"].GetString();
    
    int precision = 3;
    if (request.HasMember("precision") && request["precision"].IsInt())
        precision = request["precision"].GetInt();
    if (precision <= 0)
        precision = 3;

    int exponent_size = 3;
    if (request.HasMember("exponent_size") && request["exponent_size"].IsInt())
        exponent_size = request["exponent_size"].GetInt();

    AngleMeasure default_angle_measure = AngleMeasure::None;
    if (request.HasMember("default_angle_measure") && request["default_angle_measure"].IsInt())
        default_angle_measure = (AngleMeasure)request["default_angle_measure"].GetInt();

    AngleMeasure result_angle_measure = AngleMeasure::None;
    if (request.HasMember("result_angle_measure") && request["result_angle_measure"].IsInt())
        result_angle_measure = (AngleMeasure)request["result_angle_measure"].GetInt();

    //solving
    Real si_res = real_parser.Parse(id, expression, dependencies, default_angle_measure, result_angle_measure, precision);
    Real res = real_parser.GetSuitableUnit(id, si_res);

    bool mantissa_sign;
    std::string mantissa;
    bool exponent_sign;
    std::string exponent;
    res.ToString(exponent_size, precision, mantissa_sign, mantissa, exponent_sign, exponent);
    if (mantissa_sign)
        mantissa.insert(mantissa.begin(), '-');
    
    rapidjson::Value m(rapidjson::kStringType);
    auto& alloc = reply.GetAllocator();
    reply.AddMember("result_type", (int)ResultType::REAL, alloc);
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

    AddUnit(reply, res.unit);

    if (!si_res.unit.IsEmpty())
    {
        std::vector<Unit> cast_units;
        real_parser.GetCastUnits(id, si_res, cast_units);
        AddCastUnits(reply, cast_units);
    }

    if (res.angle_measure != AngleMeasure::None)
        reply.AddMember("angle_measure", (int)res.angle_measure, alloc);
    AddDependencies(reply, dependencies);
}

void CalculatorSolver::SolveInteger(const rapidjson::Document& request, rapidjson::Document& reply, std::vector<std::u32string>& dependencies)
{
    ElementId id;
    if (!GetElementId(request, id))
    {
        logger->Error("id error");
        throw ServiceException{ErrorCode::NO_FIELD_ERROR};
    }

    std::string expression = request["expression"].GetString();

    yutovo_calculator::Integer res = integer_parser.Parse(id, expression, dependencies);

    auto& alloc = reply.GetAllocator();
    reply.AddMember("result_type", (int)ResultType::INTEGER, alloc);
    std::string s;
    if (request.HasMember("result_notation") && request["result_notation"].IsInt())
    {
        Notation notation = (Notation)request["result_notation"].GetInt();
        if ((int)notation < 0 || notation > Notation::Hexadecimal)
            notation = Notation::Decimal;
        switch (notation)
        {
        case Notation::Binary:
            s = res.ToStdString(2);
            break;
        case Notation::Octal:
            s = res.ToStdString(8);
            break;
        case Notation::Decimal:
            s = res.ToStdString(10);
            break;
        case Notation::Hexadecimal:
            s = res.ToStdString(16);
            break;
        }
        reply.AddMember("notation", (int)notation, alloc);
    }
    else
    {
        s = res.ToStdString();
        reply.AddMember("notation", (int)Notation::Decimal, alloc);
    }
    
    rapidjson::Value val(rapidjson::kStringType);
    val.SetString(s.c_str(), s.size(), alloc);
    reply.AddMember("value", val, alloc);

    AddDependencies(reply, dependencies);
}

void CalculatorSolver::SolveRational(const rapidjson::Document& request, rapidjson::Document& reply, std::vector<std::u32string>& dependencies)
{
    ElementId id;
    if (!GetElementId(request, id))
    {
        logger->Error("id error");
        throw ServiceException{ErrorCode::NO_FIELD_ERROR};
    }

    std::string expression = request["expression"].GetString();

    Rational si_res = rational_parser.Parse(id, expression, dependencies);
    Rational res = rational_parser.GetSuitableUnit(id, si_res);

    auto& alloc = reply.GetAllocator();
    reply.AddMember("result_type", (int)ResultType::RATIONAL, alloc);
    FractionForm form = FractionForm::Improper;
    if (request.HasMember("fraction_form") && request["fraction_form"].IsInt())
        form = (FractionForm)request["fraction_form"].GetInt();

    if (form == FractionForm::Proper)
    {
        yutovo_calculator::Integer i, n, d;
        res.ToProper(i, n, d);

        if (i != 0)
        {
            std::string integer = i.ToStdString();
            rapidjson::Value _i(rapidjson::kStringType);
            _i.SetString(integer.c_str(), integer.size(), alloc);
            reply.AddMember("integer", _i, alloc);
        }

        std::string numerator = n.ToStdString();
        std::string denomerator = d.ToStdString();
        rapidjson::Value _n(rapidjson::kStringType);
        _n.SetString(numerator.c_str(), numerator.size(), alloc);
        reply.AddMember("numerator", _n, alloc);
        rapidjson::Value _d(rapidjson::kStringType);
        _d.SetString(denomerator.c_str(), denomerator.size(), alloc);
        reply.AddMember("denomerator", _d, alloc);
    }
    else
    {
        std::string numerator = res.GetNumerator().ToStdString();
        std::string denomerator = res.GetDenomerator().ToStdString();
        rapidjson::Value n(rapidjson::kStringType);
        n.SetString(numerator.c_str(), numerator.size(), alloc);
        reply.AddMember("numerator", n, alloc);
        rapidjson::Value d(rapidjson::kStringType);
        d.SetString(denomerator.c_str(), denomerator.size(), alloc);
        reply.AddMember("denomerator", d, alloc);
    }

    AddUnit(reply, res.unit);

    if (!si_res.unit.IsEmpty())
    {
        std::vector<Unit> cast_units;
        rational_parser.GetCastUnits(id, si_res, cast_units);
        AddCastUnits(reply, cast_units);
    }

    AddDependencies(reply, dependencies);
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
