/*
 * Yutovo Solver
 * Copyright (C) 2022-2026 Yutovo developers. All rights reserved.
 * This file is a part of the Yutovo project
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <memory>
#include <thread>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include "types.h"
#include "mock.h"

namespace yutovo_test
{

//The real parser does not know simplify, the symbolic parser parses it as its own function and reports a wrong arguments count
TEST_F(SolverTest, auto_simplify_shows_symbolic_parser_error)
{
    auto request = MakeAutoRequest("simplify()");
    rapidjson::Document reply;
    solver->Solve(request, reply);

    ASSERT_TRUE(reply.HasMember("error")) << "Expected a parser error reply";
    const auto& error = reply["error"];
    ASSERT_EQ(error["error_code"].GetInt(), static_cast<int>(ErrorCode::PARSER_ERROR));
    ASSERT_EQ(error["parser_error_code"].GetInt(), static_cast<int>(ParserExceptionCode::WrongArgumentsCount));
    ASSERT_EQ(error["pos"].GetInt(), 0);
}

//sin is a real parser function, so the wrong arguments count error comes from the real parser
TEST_F(SolverTest, auto_wrong_arguments_count_keeps_real_parser_error)
{
    auto request = MakeAutoRequest("sin(1,2)");
    rapidjson::Document reply;
    solver->Solve(request, reply);

    ASSERT_TRUE(reply.HasMember("error")) << "Expected a parser error reply";
    const auto& error = reply["error"];
    ASSERT_EQ(error["error_code"].GetInt(), static_cast<int>(ErrorCode::PARSER_ERROR));
    ASSERT_EQ(error["parser_error_code"].GetInt(), static_cast<int>(ParserExceptionCode::WrongArgumentsCount));
    ASSERT_TRUE(error["pos"].GetInt() == 0);
}

//The real parser parses the operator and stops at the missing operand
TEST_F(SolverTest, auto_incomplete_expression_keeps_real_parser_error)
{
    auto request = MakeAutoRequest("1 +");
    rapidjson::Document reply;
    solver->Solve(request, reply);

    ASSERT_TRUE(reply.HasMember("error")) << "Expected a parser error reply";
    const auto& error = reply["error"];
    ASSERT_EQ(error["error_code"].GetInt(), static_cast<int>(ErrorCode::PARSER_ERROR));
    ASSERT_TRUE(error["pos"].GetInt() == 3);
}

//foo is unknown for both parsers, the real parser is tried first and its error is kept
TEST_F(SolverTest, auto_unknown_function_keeps_real_parser_error)
{
    auto request = MakeAutoRequest("foo(1+2)");
    rapidjson::Document reply;
    solver->Solve(request, reply);

    ASSERT_TRUE(reply.HasMember("error")) << "Expected a parser error reply";
    const auto& error = reply["error"];
    ASSERT_EQ(error["error_code"].GetInt(), static_cast<int>(ErrorCode::PARSER_ERROR));
    ASSERT_EQ(error["parser_error_code"].GetInt(), static_cast<int>(ParserExceptionCode::UnknownIdentifier));
    ASSERT_EQ(error["pos"].GetInt(), 0);
}

}
