#include "solver.h"
#include "config.h"
#include <yutovo_calculator/integer.h>

namespace yutovo_service
{

//Solver

Solver::Solver(const std::string& _guid, const yutovo_calculator::Language _language, const char _decimal_point) :
    guid(_guid),
    locale{_language, _decimal_point}
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

void Solver::ReplyError(const yutovo_calculator::ParserException& ex, rapidjson::Document& reply)
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

bool Solver::GetUnit(const rapidjson::Document& request, Unit& unit)
{
    if (!request.HasMember("unit") || !request["unit"].IsObject())
        return false;
    
    rapidjson::Value _unit = ((rapidjson::Document&)request)["unit"].GetObject();
    if (!_unit.HasMember("value") || !_unit["value"].IsArray())
        return false;
    if (_unit.HasMember("system"))
        unit.system = ToUtfString(_unit["system"].GetString());
    rapidjson::GenericArray arr = _unit["value"].GetArray();
    for (rapidjson::SizeType i = 0; i < arr.Size(); ++i)
    {
        if (!arr[i].IsObject())
            return false;
        rapidjson::Value u = arr[i].GetObject();
        std::u32string name;
        int power = 1;
        if (!u.HasMember("name") || !u["name"].IsString())
            return false;
        name = ToUtfString(u["name"].GetString());
        if (u.HasMember("power") && u["power"].IsInt())
            power = u["power"].GetInt();
        unit.unit.push_back(std::make_pair(name, power));
    }
    return true;
}

//CalculatorSolver

CalculatorSolver::CalculatorSolver(const std::string& _guid, const yutovo_calculator::Language _language, char _decimal_point) :
    Solver(_guid, _language, _decimal_point),
    real_parser(0, _language, _decimal_point),
    integer_parser(0, _language, _decimal_point),
    rational_parser(0, _language, _decimal_point),
    complex_parser(0, _language, _decimal_point),
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
            std::vector<ResultType> results_order;
            bool exit_on_success = true;
            if (!request.HasMember("results_order") || !request["results_order"].IsArray())
            {
                results_order.push_back(ResultType::REAL);
                results_order.push_back(ResultType::INTEGER);
                results_order.push_back(ResultType::RATIONAL);
                results_order.push_back(ResultType::COMPLEX);
                exit_on_success = false;
            }
            else
            {
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
                        if (exit_on_success)
                            return;
                        break;
                    case ResultType::INTEGER:
                        SolveInteger(request, reply, dependencies);
                        if (exit_on_success)
                            return;
                        break;
                    case ResultType::RATIONAL:
                        SolveRational(request, reply, dependencies);
                        if (exit_on_success)
                            return;
                        break;
                    case ResultType::COMPLEX:
                        SolveComplex(request, reply, dependencies);
                        if (exit_on_success)
                            return;
                        break;
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
    case ResultType::COMPLEX:
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
            case ResultType::COMPLEX:
                SolveComplex(request, reply, dependencies);
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
        complex_parser.RemoveIdentifier(id, identifier);
    }
    catch (yutovo_calculator::ParserException ex)
    {
        ReplyError(ex, reply);
        return;
    }
}

void CalculatorSolver::ListIdentifiers(const rapidjson::Document& request, rapidjson::Document& reply)
{
    idle_time = time(nullptr);

    //collect all the identifiers from all the parsers
    std::vector<std::u32string> builtin_functions, builtin_variables, builtin_units;
    real_parser.ListBuiltinFunctions(builtin_functions);
    real_parser.ListBuiltinVariables(builtin_variables);
    real_parser.ListBuiltinUnits(builtin_units);

    integer_parser.ListBuiltinFunctions(builtin_functions);
    integer_parser.ListBuiltinVariables(builtin_variables);

    rational_parser.ListBuiltinFunctions(builtin_functions);
    rational_parser.ListBuiltinVariables(builtin_variables);
    rational_parser.ListBuiltinUnits(builtin_units);

    complex_parser.ListBuiltinFunctions(builtin_functions);
    complex_parser.ListBuiltinVariables(builtin_variables);

    //remove duplicates
    std::sort(builtin_functions.begin(), builtin_functions.end());
    builtin_functions.erase(std::unique(builtin_functions.begin(), builtin_functions.end()), builtin_functions.end());

    std::sort(builtin_variables.begin(), builtin_variables.end());
    builtin_variables.erase(std::unique(builtin_variables.begin(), builtin_variables.end()), builtin_variables.end());

    std::sort(builtin_units.begin(), builtin_units.end());
    builtin_units.erase(std::unique(builtin_units.begin(), builtin_units.end()), builtin_units.end());

    auto& alloc = reply.GetAllocator();
    rapidjson::Value builtin_functions_arr(rapidjson::kArrayType);
    for (auto& f : builtin_functions)
    {
        rapidjson::Value func;
        func.SetObject();
        func.AddMember("name", rapidjson::Value((boost::locale::conv::utf_to_utf<char>(f)).c_str(), alloc), alloc);
        builtin_functions_arr.PushBack(func, alloc);
    }
    reply.AddMember("builtin_functions", builtin_functions_arr, alloc);

    rapidjson::Value builtin_variables_arr(rapidjson::kArrayType);
    for (auto& u : builtin_variables)
    {
        rapidjson::Value var;
        var.SetObject();
        var.AddMember("name", rapidjson::Value((boost::locale::conv::utf_to_utf<char>(u)).c_str(), alloc), alloc);
        builtin_variables_arr.PushBack(var, alloc);
    }
    reply.AddMember("builtin_variables", builtin_variables_arr, alloc);

    rapidjson::Value builtin_units_arr(rapidjson::kArrayType);
    for (auto& u : builtin_units)
    {
        rapidjson::Value var;
        var.SetObject();
        var.AddMember("name", rapidjson::Value((boost::locale::conv::utf_to_utf<char>(u)).c_str(), alloc), alloc);
        builtin_units_arr.PushBack(var, alloc);
    }
    reply.AddMember("builtin_units", builtin_units_arr, alloc);

    ListUserIdentifiers(request, reply);
}

void CalculatorSolver::ListUserIdentifiers(const rapidjson::Document& request, rapidjson::Document& reply)
{
    idle_time = time(nullptr);

    //collect all the identifiers from all the parsers
    std::vector<std::u32string> user_functions, user_variables, user_units;
    real_parser.ListUserFunctions(user_functions);
    real_parser.ListUserVariables(user_variables);
    real_parser.ListUserUnits(user_units);

    integer_parser.ListUserFunctions(user_functions);
    integer_parser.ListUserVariables(user_variables);

    rational_parser.ListUserFunctions(user_functions);
    rational_parser.ListUserVariables(user_variables);
    rational_parser.ListUserUnits(user_units);

    complex_parser.ListUserFunctions(user_functions);
    complex_parser.ListUserVariables(user_variables);

    //remove duplicates
    std::sort(user_functions.begin(), user_functions.end());
    user_functions.erase(std::unique(user_functions.begin(), user_functions.end()), user_functions.end());

    std::sort(user_variables.begin(), user_variables.end());
    user_variables.erase(std::unique(user_variables.begin(), user_variables.end()), user_variables.end());

    std::sort(user_units.begin(), user_units.end());
    user_units.erase(std::unique(user_units.begin(), user_units.end()), user_units.end());

    auto& alloc = reply.GetAllocator();
    rapidjson::Value user_functions_arr(rapidjson::kArrayType);
    for (auto& f : user_functions)
    {
        rapidjson::Value func;
        func.SetObject();
        func.AddMember("name", rapidjson::Value((boost::locale::conv::utf_to_utf<char>(f)).c_str(), alloc), alloc);
        user_functions_arr.PushBack(func, alloc);
    }
    reply.AddMember("user_functions", user_functions_arr, alloc);

    rapidjson::Value user_variables_arr(rapidjson::kArrayType);
    for (auto& u : user_variables)
    {
        rapidjson::Value var;
        var.SetObject();
        var.AddMember("name", rapidjson::Value((boost::locale::conv::utf_to_utf<char>(u)).c_str(), alloc), alloc);
        user_variables_arr.PushBack(var, alloc);
    }
    reply.AddMember("user_variables", user_variables_arr, alloc);

    rapidjson::Value user_units_arr(rapidjson::kArrayType);
    for (auto& u : user_units)
    {
        rapidjson::Value var;
        var.SetObject();
        var.AddMember("name", rapidjson::Value((boost::locale::conv::utf_to_utf<char>(u)).c_str(), alloc), alloc);
        user_units_arr.PushBack(var, alloc);
    }
    reply.AddMember("user_units", user_units_arr, alloc);
}

bool CalculatorSolver::SetLocale(const rapidjson::Document& request, rapidjson::Document& reply)
{
    idle_time = time(nullptr);

    if (!request.HasMember("language") || !request["language"].IsInt())
    {
        logger->Error("language error");
        ReplyError(ErrorCode::NO_FIELD_ERROR, reply);
        return false;
    }

    locale.language = (Language)request["language"].GetInt();
    if (request.HasMember("decimal_point") || request["decimal_point"].IsString())
    {
        std::string p = request["decimal_point"].GetString();
        if (p.length() == 1)
            locale.decimal_point = p[0];
    }

    try
    {
        real_parser.SetLocale(locale.language, locale.decimal_point);
        integer_parser.SetLocale(locale.language, locale.decimal_point);
        rational_parser.SetLocale(locale.language, locale.decimal_point);
        complex_parser.SetLocale(locale.language, locale.decimal_point);
    }
    catch (yutovo_calculator::ParserException ex)
    {
        ReplyError(ex, reply);
        return false;
    }

    return true;
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
    Real res;

    Unit unit;
    if (GetUnit(request, unit))
        res = real_parser.CastToUnit(id, si_res, unit);
    else
        res = real_parser.GetSuitableUnit(id, si_res);

    bool mantissa_sign;
    std::string mantissa;
    bool exponent_sign;
    std::string exponent;
    res.ToString(exponent_size, precision, mantissa_sign, mantissa, exponent_sign, exponent, locale.decimal_point);
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
    Notation default_notation = Notation::Decimal;

    if (request.HasMember("default_notation") && request["default_notation"].IsInt())
        default_notation = (Notation)request["default_notation"].GetInt();

    yutovo_calculator::Integer res = integer_parser.Parse(id, expression, dependencies, default_notation);

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
    Rational res;

    Unit unit;
    if (GetUnit(request, unit))
        res = rational_parser.CastToUnit(id, si_res, unit);
    else
        res = rational_parser.GetSuitableUnit(id, si_res);

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

void CalculatorSolver::SolveComplex(const rapidjson::Document& request, rapidjson::Document& reply, std::vector<std::u32string>& dependencies)
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
    
    ComplexForm form = ComplexForm::Arithmetic;
    if (request.HasMember("form") && request["form"].IsInt())
        form = (ComplexForm)request["form"].GetInt();
    
    int max_count = 10;
    if (request.HasMember("max_count") && request["max_count"].IsInt())
        max_count = request["max_count"].GetInt();

    //solving
    std::vector<Complex> results;
    complex_parser.Parse(id, expression, dependencies, default_angle_measure, result_angle_measure, precision, max_count, results);

    auto& alloc = reply.GetAllocator();
    reply.AddMember("result_type", (int)ResultType::COMPLEX, alloc);

    rapidjson::Value results_arr(rapidjson::kArrayType);
    for (Complex& r : results)
    {
        rapidjson::Value res;
        res.SetObject();
        if (form == ComplexForm::Trigonometric || form == ComplexForm::Exponential)
        {
            rapidjson::Value m(rapidjson::kObjectType);
            AddReal(reply, m, module(r), exponent_size, precision);
            res.AddMember("module", m, alloc);

            rapidjson::Value a(rapidjson::kObjectType);
            AddReal(reply, a, argument(r), exponent_size, precision);
            res.AddMember("argument", a, alloc);
        }
        else
        {
            if (r.GetRe() != 0)
            {
                rapidjson::Value re(rapidjson::kObjectType);
                AddReal(reply, re, r.GetRe(), exponent_size, precision);
                res.AddMember("re", re, alloc);
            }

            if (r.GetIm() != 0)
            {
                rapidjson::Value im(rapidjson::kObjectType);
                AddReal(reply, im, r.GetIm(), exponent_size, precision);
                res.AddMember("im", im, alloc);
            }
        }

        if (r.GetAngleMeasure() != AngleMeasure::None)
            res.AddMember("angle_measure", (int)r.GetAngleMeasure(), alloc);

        results_arr.PushBack(res, alloc);
    }

    reply.AddMember("results", results_arr, alloc);

    AddDependencies(reply, dependencies);
}

void CalculatorSolver::AddReal(rapidjson::Document& reply, rapidjson::Value& obj, const Real& value, const int exponent_size, const int precision)
{
    bool mantissa_sign;
    std::string mantissa;
    bool exponent_sign;
    std::string exponent;
    value.ToString(exponent_size, precision, mantissa_sign, mantissa, exponent_sign, exponent, locale.decimal_point);
    if (mantissa_sign)
        mantissa.insert(mantissa.begin(), '-');
    
    auto& alloc = reply.GetAllocator();
    rapidjson::Value m(rapidjson::kStringType);
    m.SetString(mantissa.c_str(), mantissa.size(), alloc);

    obj.AddMember("mantissa", m, alloc);
    if (exponent_sign)
        exponent.insert(exponent.begin(), '-');
    if (exponent != "")
    {
        rapidjson::Value e(rapidjson::kStringType);
        e.SetString(exponent.c_str(), exponent.size(), alloc);
        obj.AddMember("exponent", e, alloc);
    }
}

//PythonSolver

PythonSolver::PythonSolver(const std::string& _guid, const yutovo_calculator::Language _language) :
    Solver(_guid, _language, U'.'),
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

void PythonSolver::ListIdentifiers(const rapidjson::Document& request, rapidjson::Document& reply)
{
}

void PythonSolver::ListUserIdentifiers(const rapidjson::Document& request, rapidjson::Document& reply)
{
}

bool PythonSolver::SetLocale(const rapidjson::Document& request, rapidjson::Document& reply)
{
    return true;
}

//Solvers

Solvers::Solvers(Config* _config) :
    config(_config)
{
}

SolverPtr Solvers::GetSolver(const std::string& guid, const int code_id, SolverType solver_type)
{
    SolverLocale locale;

    std::lock_guard<std::mutex> lock(solvers_mutex);
    auto it = solvers.find(guid);
    if (it != solvers.end())
    {
        auto it_c = it->second.find(code_id);
        if (it_c != it->second.end())
            return it_c->second;
    }
    
    auto it_l = solvers_locales.find(guid);
    if (it_l != solvers_locales.end())
        locale = it_l->second;
    
    switch (solver_type)
    {
    case SolverType::CALCULATOR:
        {
            std::string solver_id = guid + "-" + std::to_string(code_id);
            SolverPtr solver(new CalculatorSolver(solver_id, locale.language, locale.decimal_point));
            if (it == solvers.end())
            {
                std::map<int, SolverPtr> m;
                m[code_id] = solver;
                solvers[guid] = m;
            }
            else
            {
                it->second[code_id] = solver;
            }
            return solver;
        }
    case SolverType::PYTHON:
        break;
    }

    return nullptr;
}

void Solvers::SetLocale(const std::string& guid, const yutovo_calculator::Language language, const char decimal_point, 
    const rapidjson::Document& request, rapidjson::Document& reply)
{
    std::lock_guard<std::mutex> lock(solvers_mutex);
    auto it = solvers.find(guid);
    if (it != solvers.end())
    {
        for (auto& [code_id, solver] : it->second)
            solver->SetLocale(request, reply);
    }

    solvers_locales[guid] = SolverLocale{language, decimal_point};
}

void Solvers::RemoveTimeouted()
{
    std::lock_guard<std::mutex> lock(solvers_mutex);
    for (auto it = solvers.begin(); it != solvers.end(); ++it)
    {
        auto& m = it->second;
        for (auto it_s = m.begin(); it_s != m.end();)
        {
            SolverPtr& s = it_s->second;
            if (time(nullptr) - s->idle_time > config->solver_idle_timeout)
            {
                m.erase(it_s++);
                continue;
            }
            ++it_s;
        }
    }
}

}
