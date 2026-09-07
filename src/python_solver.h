/*
 * Yutovo Solver
 * Copyright (C) 2022-2026 Yutovo developers. All rights reserved.
 * This file is a part of the Yutovo project
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef __PYTHON_SOLVER_H__
#define __PYTHON_SOLVER_H__

#include "service_solver.h"

namespace yutovo_solver
{

class PythonSolver : public Solver
{
public:
    PythonSolver(const std::string& _document_guid, const std::string& _solver_guid, const yutovo_calculator::Language _language, uint64_t _max_time,
        const std::string& _logs_path, bool _log_console, bool _log_file);
    ~PythonSolver();

    virtual void Solve(const rapidjson::Document& request, rapidjson::Document& reply);
    virtual void BreakSolving(const rapidjson::Document& request, rapidjson::Document& reply);
    virtual void RemoveIdentifier(const rapidjson::Document& request, rapidjson::Document& reply);
    virtual void RemoveUserIdentifiers(const rapidjson::Document& request, rapidjson::Document& reply);
    virtual void ListIdentifiers(const rapidjson::Document& request, rapidjson::Document& reply);
    virtual bool SetLocale(const rapidjson::Document& request, rapidjson::Document& reply);

private:
    Logger* logger = nullptr;
};

}

#endif
