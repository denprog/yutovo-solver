/*
 * Yutovo Solver
 * Copyright (C) 2022-2026 Yutovo developers. All rights reserved.
 * This file is a part of the Yutovo project
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef __CALCULATOR_SOLVER_H__
#define __CALCULATOR_SOLVER_H__

#include "service_solver.h"
#include <yutovo-calculator/symbolic.h>

namespace yutovo_solver
{

class SolverProcess;

class CalculatorSolver : public Solver
{
public:
    CalculatorSolver(const std::string& _document_guid, const std::string& _solver_guid, ParserContextPtr _parser_context,
        const yutovo_calculator::Language _language, uint64_t _max_time, const std::string& _logs_path, bool _log_console, bool _log_file);
    ~CalculatorSolver();

    virtual void Solve(const rapidjson::Document& request, rapidjson::Document& reply);
    virtual void BreakSolving(const rapidjson::Document& request, rapidjson::Document& reply);
    virtual void RemoveIdentifier(const rapidjson::Document& request, rapidjson::Document& reply);
    virtual void RemoveUserIdentifiers(const rapidjson::Document& request, rapidjson::Document& reply);
    virtual void ListIdentifiers(const rapidjson::Document& request, rapidjson::Document& reply);
    virtual bool SetLocale(const rapidjson::Document& request, rapidjson::Document& reply);
    virtual void SetMaxTime(const uint64_t _max_time);

private:
    void SolveReal(const rapidjson::Document& request, rapidjson::Document& reply, std::vector<std::u32string>* dependencies);
    void SolveInteger(const rapidjson::Document& request, rapidjson::Document& reply, std::vector<std::u32string>* dependencies);
    void SolveRational(const rapidjson::Document& request, rapidjson::Document& reply, std::vector<std::u32string>* dependencies);
    void SolveComplex(const rapidjson::Document& request, rapidjson::Document& reply, std::vector<std::u32string>* dependencies);
    void SolveArrayReal(const rapidjson::Document& request, rapidjson::Document& reply, std::vector<std::u32string>* dependencies);
    void SolveSymbolicReal(const rapidjson::Document& request, rapidjson::Document& reply, std::vector<std::u32string>* dependencies);
    void SolveSymbolicRational(const rapidjson::Document& request, rapidjson::Document& reply, std::vector<std::u32string>* dependencies);
    void SolveSymbolicComplex(const rapidjson::Document& request, rapidjson::Document& reply, std::vector<std::u32string>* dependencies);

    void AddReal(rapidjson::Document& reply, rapidjson::Value& obj, const Real& value, const int exponent_size, const int precision);

private:
    std::mutex parsers_lock;
    yutovo_calculator::Parser<yutovo_calculator::Real> real_parser;
    yutovo_calculator::Parser<yutovo_calculator::Integer> integer_parser;
    yutovo_calculator::Parser<yutovo_calculator::Rational> rational_parser;
    yutovo_calculator::Parser<yutovo_calculator::Complex> complex_parser;
    yutovo_calculator::Parser<yutovo_calculator::Array<Real>> array_real_parser;
    yutovo_calculator::Parser<yutovo_calculator::Symbolic<Real>> symbolic_real_parser;
    yutovo_calculator::Parser<yutovo_calculator::Symbolic<Rational>> symbolic_rational_parser;
    yutovo_calculator::Parser<yutovo_calculator::Symbolic<Complex>> symbolic_complex_parser;

    ParserContextPtr parser_context;

#ifdef YUTOVO_SOLVER_WORKER
    //in proxy mode parser_context is null and the parsers live in the calculator worker process
    std::shared_ptr<SolverProcess> process;
#endif

    std::mutex solving_id_lock;
    LogicalId solving_id;
    uint64_t solving_time_stamp = 0;

    std::mutex break_lock;
    std::map<LogicalId, int64_t> break_solvings;

    std::atomic<bool> just_started = true;

    Logger* logger = nullptr;
};

class SolveTimeoutWatchdog
{
public:
    SolveTimeoutWatchdog(uint64_t _max_time);
    ~SolveTimeoutWatchdog();

    std::atomic<bool> fired{false};

private:
    std::atomic<bool> done{false};
    std::thread timeout_thread;

    //gives the in-parser CPU timer a chance to fire first, then interrupts a giac evaluation stuck past the deadline
    const uint64_t interrupt_reserve_ms = 250;
};

}

#endif
