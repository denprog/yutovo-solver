/*
 * Yutovo Solver
 * Copyright (C) 2022-2026 Yutovo developers. All rights reserved.
 * This file is a part of the Yutovo project
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "service_solver.h"
#include "calculator_solver.h"
#include "python_solver.h"
#include "service_config.h"

#ifdef YUTOVO_SOLVER_WORKER
#include "solver_process.h"
#endif

#ifdef _MSC_VER
#undef GetObject
#endif

namespace yutovo_solver
{

//Solver

Solver::Solver(const std::string& _document_guid, const std::string& _solver_guid, const yutovo_calculator::Language _language) :
    document_guid(_document_guid), 
    solver_guid(_solver_guid),
    locale{_language}
{
}

void Solver::SetMaxTime(const uint64_t _max_time)
{
    max_time = _max_time;
}

void Solver::ReplyOk(rapidjson::Document& reply)
{
    rapidjson::Value ok;
    ok.SetObject();
    auto& alloc = reply.GetAllocator();
    ok.AddMember("error_code", (int)ErrorCode::OK, alloc);
    reply.AddMember("result", ok, alloc);
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
    error.AddMember("id", LogicalIdToValue(reply, ex.id), alloc);
    error.AddMember("error_code", (int)ErrorCode::PARSER_ERROR, alloc);
    error.AddMember("parser_error_code", ex.ex_id, alloc);
    error.AddMember("pos", ex.pos, alloc);
    error.AddMember("size", ex.size, alloc);
    error.AddMember("line", ex.line, alloc);
    rapidjson::Value s((boost::locale::conv::utf_to_utf<char>(ex.description)).c_str(), alloc);
    error.AddMember("description", s, alloc);
    reply.AddMember("error", error, alloc);
}

void Solver::AddUnit(rapidjson::Value& reply, const Unit& unit, rapidjson::Document::AllocatorType& alloc)
{
    if (unit.IsEmpty())
        return;

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

void Solver::AddDependencies(rapidjson::Document& reply, const std::vector<std::u32string>* dependencies)
{
    if (dependencies->empty())
        return;
    
    auto& alloc = reply.GetAllocator();
    rapidjson::Value d(rapidjson::kArrayType);
    for (auto& str : *dependencies)
    {
        rapidjson::Value s((boost::locale::conv::utf_to_utf<char>(str)).c_str(), alloc);
        d.PushBack(s, alloc);
    }
    while (reply.HasMember("dependencies")) //remove the previous dependencies added by former tries
        reply.RemoveMember("dependencies");
    reply.AddMember("dependencies", d, alloc);
}

bool Solver::GetLogicalId(const rapidjson::Document& request, LogicalId& id)
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

bool Solver::GetTimestamp(const rapidjson::Document& request, uint64_t& time_stamp)
{
    if (!request.HasMember("timestamp") || !request["timestamp"].IsUint64())
        return false;
    time_stamp = request["timestamp"].GetUint64();
    return true;
}

rapidjson::Value Solver::LogicalIdToValue(rapidjson::Document& reply, const LogicalId& id)
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
    
    const auto& _unit = request["unit"].GetObject();
    if (!_unit.HasMember("value") || !_unit["value"].IsArray())
        return false;
    if (_unit.HasMember("system"))
        unit.system = ToUtfString(_unit["system"].GetString());
    rapidjson::GenericArray arr = _unit["value"].GetArray();
    for (rapidjson::SizeType i = 0; i < arr.Size(); ++i)
    {
        if (!arr[i].IsObject())
            return false;
        const auto& u = arr[i].GetObject();
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

//Solvers

std::map<std::string, ExportPtr> Solvers::document_exports;

Solvers::Solvers(ServiceConfig* _service_config, const std::string& _logs_path, bool _log_console, bool _log_file) :
    service_config(_service_config),
    logs_path(_logs_path), 
    log_console(_log_console), 
    log_file(_log_file)
{
}

SolverPtr Solvers::GetSolver(const std::string& document_guid, const std::string& solver_guid, const int code_id, SolverType solver_type)
{
    SolverLocale locale;

    std::lock_guard<std::mutex> lock(solvers_mutex);
    auto it = solvers.find(solver_guid);
    if (it != solvers.end())
    {
        auto it_c = it->second.find(code_id);
        if (it_c != it->second.end())
            return it_c->second;
    }
    
    auto it_l = solvers_locales.find(solver_guid);
    if (it_l != solvers_locales.end())
        locale = it_l->second;

    ParserContextPtr parser_context;
    //on desktop the parsers live in a separate worker process, so a proxy solver gets no parser context
#ifndef YUTOVO_SOLVER_WORKER
    ExportPtr exports;
    auto it_e = document_exports.find(document_guid);
    if (it_e != document_exports.end())
        exports = it_e->second;
    else
    {
        exports.reset(new yutovo_calculator::Export());
        document_exports[document_guid] = exports;
    }
    //each solver gets its own parser context, so concurrent solvings of one document don't race on its fields, only the exports are shared
    parser_context.reset(new yutovo_calculator::ParserContext());
    parser_context->exports = exports;
#endif
    
    switch (solver_type)
    {
    case SolverType::CALCULATOR:
        {
            std::string solver_id = solver_guid + "-" + std::to_string(code_id);
            SolverPtr solver(new CalculatorSolver(document_guid, solver_id, parser_context, locale.language, service_config->max_time, 
                logs_path, log_console, log_file));
            if (it == solvers.end())
            {
                std::map<int, SolverPtr> m;
                m[code_id] = solver;
                solvers[solver_guid] = m;
            }
            else
            {
                it->second[code_id] = solver;
            }
            return solver;
        }
    case SolverType::PYTHON:
    case SolverType::NONE:
        break;
    }

    return nullptr;
}

bool Solvers::RemoveSolver(const std::string& solver_guid, const int code_id)
{
    std::lock_guard<std::mutex> lock(solvers_mutex);
    auto it = solvers.find(solver_guid);
    if (it != solvers.end())
    {
        auto it_c = it->second.find(code_id);
        if (it_c != it->second.end())
        {
            it->second.erase(it_c);
            return true;
        }
    }
    return false;
}

void Solvers::SetLocale(const std::string& solver_guid, const yutovo_calculator::Language language, 
    const rapidjson::Document& request, rapidjson::Document& reply)
{
    //an out of range language would throw from the parser constructors when a solver is created
    if (language < yutovo_calculator::Language::English || language > yutovo_calculator::Language::BrazilianPortuguese)
        return;

    std::lock_guard<std::mutex> lock(solvers_mutex);
    auto it = solvers.find(solver_guid);
    if (it != solvers.end())
    {
        for (auto& [code_id, solver] : it->second)
            solver->SetLocale(request, reply);
    }

    solvers_locales[solver_guid] = SolverLocale{language};
}

void Solvers::RemoveUserIdentifiers(const std::string& solver_guid, const rapidjson::Document& request, rapidjson::Document& reply)
{
    std::lock_guard<std::mutex> lock(solvers_mutex);
    auto it = solvers.find(solver_guid);
    if (it != solvers.end())
    {
        for (auto& [code_id, solver] : it->second)
            solver->RemoveUserIdentifiers(request, reply);
    }
}

void Solvers::ClearExport(const std::string& document_guid, const rapidjson::Document& request, rapidjson::Document& reply)
{
    std::unique_lock<std::mutex> lock(solvers_mutex);
#ifdef YUTOVO_SOLVER_WORKER
    //the parser context lives in the calculator worker process of the document
    auto process = SolverProcess::FindProcess(document_guid);
    if (process)
    {
        lock.unlock();
        process->SendAction("", "clear_export", request, reply);
    }
#else
    auto it = document_exports.find(document_guid);
    if (it != document_exports.end())
        it->second->Clear();
#endif
}

void Solvers::SetMaxTime(const uint64_t max_time)
{
    //new solvers are created with service_config->max_time, so it must be updated as well
    service_config->max_time = max_time;
    std::lock_guard<std::mutex> lock(solvers_mutex);
    for (auto& s : solvers)
    {
        for (auto& solver : s.second)
            solver.second->SetMaxTime(max_time);
    }
}

void Solvers::RemoveTimeouted()
{
    std::lock_guard<std::mutex> lock(solvers_mutex);
    //remove unusing solvers
    for (auto it = solvers.begin(); it != solvers.end(); ++it)
    {
        auto& m = it->second;
        for (auto it_s = m.begin(); it_s != m.end();)
        {
            SolverPtr& s = it_s->second;
            if (time(nullptr) - s->idle_time > service_config->solver_idle_timeout)
            {
                m.erase(it_s++); //remove solver
                continue;
            }
            ++it_s;
        }
    }

    //remove unusing document exports
    for (auto it_p = document_exports.begin(); it_p != document_exports.end();)
    {
        auto& document_guid = it_p->first;
        auto it = std::find_if(solvers.begin(), solvers.end(), 
            [document_guid](auto& s)
            {
                auto it_s = std::find_if(s.second.begin(), s.second.end(), 
                    [document_guid](auto& solver)
                    {
                        return solver.second->document_guid == document_guid;
                    });
                return it_s != s.second.end();
            });
        if (it == solvers.end())
            it_p = document_exports.erase(it_p);
        else
            ++it_p;
    }
}

}
