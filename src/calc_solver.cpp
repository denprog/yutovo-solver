#include "calc_solver.h"

namespace yutovo_service
{

CalcSolver::CalcSolver() :
    logger(Logger::GetInstance("programs/Math/bin", "calc_solver", true, true))
{
    logger->Info("CalcSolver start");
}

CalcSolver::~CalcSolver()
{
    logger->Info("CalcSolver end");
}

bool CalcSolver::Solve(const rapidjson::Document& request, rapidjson::Document& reply)
{
    if (!request.HasMember("result_type") || !request["result_type"].IsInt())
    {
        logger->Error("result_type error");
        return false;
    }

    if (!request.HasMember("expression") || !request["expression"].IsString())
    {
        logger->Error("expression error");
        return false;
    }

    std::string expression = request["expression"].GetString();
    int result_type = request["result_type"].GetInt();

    reply.SetObject();
    auto& alloc = reply.GetAllocator();

    switch (result_type)
    {
    case 1: //real
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
            
            yutovo_calc::Parser<yutovo_calc::Real> parser(precision);
            yutovo_calc::Real res;

            try
            {
                res = parser.Parse(expression);
            }
            catch (yutovo_calc::ParserException ex)
            {
                rapidjson::Value error;
                error.SetObject();
                error.AddMember("id", ex.id, alloc);
                error.AddMember("pos", ex.pos, alloc);
                error.AddMember("line", ex.line, alloc);
                reply.AddMember("error", error, alloc);
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
    case 2: //integer
        {
            int notation = 1;
            if (request.HasMember("notation") && request["notation"].IsInt())
                notation = request["notation"].GetInt();
            
            yutovo_calc::Parser<yutovo_calc::Integer> parser(0);
            yutovo_calc::Integer res;
            try
            {
                res = parser.Parse(expression);
            }
            catch (yutovo_calc::ParserException ex)
            {
                rapidjson::Value error;
                error.SetObject();
                error.AddMember("id", ex.id, alloc);
                error.AddMember("pos", ex.pos, alloc);
                error.AddMember("line", ex.line, alloc);
                reply.AddMember("error", error, alloc);
                break;
            }

            std::string s = res.ToString();
            rapidjson::Value val(rapidjson::kStringType);
            val.SetString(s.c_str(), s.size(), alloc);
            reply.AddMember("value", val, alloc);
        }
        break;
    case 3: //fractional
        {
            yutovo_calc::Parser<yutovo_calc::Rational> parser(0);
            yutovo_calc::Rational res;
            try
            {
                res = parser.Parse(expression);
            }
            catch (yutovo_calc::ParserException ex)
            {
                rapidjson::Value error;
                error.SetObject();
                error.AddMember("id", ex.id, alloc);
                error.AddMember("pos", ex.pos, alloc);
                error.AddMember("line", ex.line, alloc);
                reply.AddMember("error", error, alloc);
                break;
            }

            std::string numerator = res.GetNumerator().ToString();
            std::string denomerator = res.GetDenomerator().ToString();
            rapidjson::Value n(rapidjson::kStringType);
            n.SetString(numerator.c_str(), numerator.size(), alloc);
            reply.AddMember("numerator", n, alloc);
            rapidjson::Value d(rapidjson::kStringType);
            d.SetString(denomerator.c_str(), denomerator.size(), alloc);
            reply.AddMember("denomerator", d, alloc);
        }
        break;
    case 4: //complex
        break;
    }

    return true;
}

}
