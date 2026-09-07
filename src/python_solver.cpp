/*
 * Yutovo Solver
 * Copyright (C) 2022-2026 Yutovo developers. All rights reserved.
 * This file is a part of the Yutovo project
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "python_solver.h"

namespace yutovo_solver
{

//PythonSolver

PythonSolver::PythonSolver(const std::string& _document_guid, const std::string& _solver_guid, const yutovo_calculator::Language _language, 
    uint64_t _max_time, const std::string& _logs_path, bool _log_console, bool _log_file) :
    Solver(_document_guid, _solver_guid, _language),
    logger(Logger::GetInstance(_logs_path + "/yutovo-solver", "python-solver", _log_console, _log_file))
{
}

PythonSolver::~PythonSolver()
{
    logger->Info("Python Solver finished: {}", solver_guid);
}

void PythonSolver::Solve(const rapidjson::Document& request, rapidjson::Document& reply)
{
    throw ServiceException{ErrorCode::NOT_IMPLEMENTED};
}

void PythonSolver::BreakSolving(const rapidjson::Document& request, rapidjson::Document& reply)
{
    throw ServiceException{ErrorCode::NOT_IMPLEMENTED};
}

void PythonSolver::RemoveIdentifier(const rapidjson::Document& request, rapidjson::Document& reply)
{
    throw ServiceException{ErrorCode::NOT_IMPLEMENTED};
}

void PythonSolver::RemoveUserIdentifiers(const rapidjson::Document& request, rapidjson::Document& reply)
{
    throw ServiceException{ErrorCode::NOT_IMPLEMENTED};
}

void PythonSolver::ListIdentifiers(const rapidjson::Document& request, rapidjson::Document& reply)
{
    throw ServiceException{ErrorCode::NOT_IMPLEMENTED};
}

bool PythonSolver::SetLocale(const rapidjson::Document& request, rapidjson::Document& reply)
{
    throw ServiceException{ErrorCode::NOT_IMPLEMENTED};
}

}
