/*
 * Yutovo Solver
 * Copyright (C) 2022-2026 Yutovo developers. All rights reserved.
 * This file is a part of the Yutovo project
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <gtest/gtest.h>
#include <rapidjson/document.h>
#include "mock.h"

namespace yutovo_test
{

TEST_F(SolverTest, solve_real)
{
    auto request = MakeRequest(ResultType::REAL, "1+2");
    rapidjson::Document reply;
    solver->Solve(request, reply);
    ASSERT_FALSE(reply.HasMember("error")) << "Unexpected solver error";
    ASSERT_TRUE(reply.HasMember("result_type"));
    ASSERT_EQ(reply["result_type"].GetInt(), static_cast<int>(ResultType::REAL));
}

TEST_F(SolverTest, solve_integer)
{
    auto request = MakeRequest(ResultType::INTEGER, "5!");
    rapidjson::Document reply;
    solver->Solve(request, reply);
    ASSERT_FALSE(reply.HasMember("error")) << "Unexpected solver error";
    ASSERT_TRUE(reply.HasMember("result_type"));
    ASSERT_EQ(reply["result_type"].GetInt(), static_cast<int>(ResultType::INTEGER));
}

TEST_F(SolverTest, solve_rational)
{
    auto request = MakeRequest(ResultType::RATIONAL, "2/4");
    rapidjson::Document reply;
    solver->Solve(request, reply);
    ASSERT_FALSE(reply.HasMember("error")) << "Unexpected solver error";
    ASSERT_TRUE(reply.HasMember("result_type"));
    ASSERT_EQ(reply["result_type"].GetInt(), static_cast<int>(ResultType::RATIONAL));
}

TEST_F(SolverTest, solve_complex)
{
    auto request = MakeRequest(ResultType::COMPLEX, "2+1i");
    rapidjson::Document reply;
    solver->Solve(request, reply);
    ASSERT_FALSE(reply.HasMember("error")) << "Unexpected solver error";
    ASSERT_TRUE(reply.HasMember("result_type"));
    ASSERT_EQ(reply["result_type"].GetInt(), static_cast<int>(ResultType::COMPLEX));
}

TEST_F(SolverTest, solve_array_real)
{
    auto request = MakeRequest(ResultType::ARRAY_REAL, "[1,2,3]");
    rapidjson::Document reply;
    solver->Solve(request, reply);
    ASSERT_FALSE(reply.HasMember("error")) << "Unexpected solver error";
    ASSERT_TRUE(reply.HasMember("result_type"));
    ASSERT_EQ(reply["result_type"].GetInt(), static_cast<int>(ResultType::ARRAY_REAL));
}

TEST_F(SolverTest, solve_symbolic_real)
{
    auto request = MakeRequest(ResultType::SYMBOLIC_REAL, "x+1");
    rapidjson::Document reply;
    solver->Solve(request, reply);
    ASSERT_FALSE(reply.HasMember("error")) << "Unexpected solver error";
    ASSERT_TRUE(reply.HasMember("result_type"));
    ASSERT_EQ(reply["result_type"].GetInt(), static_cast<int>(ResultType::SYMBOLIC_REAL));
}

TEST_F(SolverTest, solve_symbolic_rational)
{
    auto request = MakeRequest(ResultType::SYMBOLIC_RATIONAL, "x+1/2");
    rapidjson::Document reply;
    solver->Solve(request, reply);
    ASSERT_FALSE(reply.HasMember("error")) << "Unexpected solver error";
    ASSERT_TRUE(reply.HasMember("result_type"));
    ASSERT_EQ(reply["result_type"].GetInt(), static_cast<int>(ResultType::SYMBOLIC_RATIONAL));
}

TEST_F(SolverTest, solve_symbolic_complex)
{
    auto request = MakeRequest(ResultType::SYMBOLIC_COMPLEX, "1+2*i");
    rapidjson::Document reply;
    solver->Solve(request, reply);
    ASSERT_FALSE(reply.HasMember("error")) << "Unexpected solver error";
    ASSERT_TRUE(reply.HasMember("result_type"));
    ASSERT_EQ(reply["result_type"].GetInt(), static_cast<int>(ResultType::SYMBOLIC_COMPLEX));
}

TEST_F(SolverTest, solve_auto_with_results_order)
{
    auto request = MakeRequest(ResultType::AUTO, "x+1");
    auto& alloc = request.GetAllocator();
    rapidjson::Value order(rapidjson::kArrayType);
    order.PushBack(static_cast<int>(ResultType::SYMBOLIC_REAL), alloc);
    request.AddMember("results_order", order, alloc);

    rapidjson::Document reply;
    solver->Solve(request, reply);
    ASSERT_FALSE(reply.HasMember("error")) << "Unexpected solver error";
    ASSERT_TRUE(reply.HasMember("result_type"));
    ASSERT_EQ(reply["result_type"].GetInt(), static_cast<int>(ResultType::SYMBOLIC_REAL));
}

TEST_F(SolverTest, auto_wrong_expression_returns_error_object)
{
    for (const char* expr : {";", "="})
    {
        auto request = MakeAutoRequest(expr);
        rapidjson::Document reply;
        solver->Solve(request, reply);
        ASSERT_TRUE(reply.IsObject()) << "reply for [" << expr << "] is not an object";
        ASSERT_TRUE(reply.HasMember("error")) << "expected parser error for [" << expr << "]";
        ASSERT_EQ(reply["error"]["error_code"].GetInt(), static_cast<int>(ErrorCode::PARSER_ERROR));
    }
}

TEST_F(SolverTest, solve_auto_error_dependencies)
{
    auto make_request_with_id =
        [](ResultType result_type, const char* expression, std::vector<int> logical_id)
    {
        rapidjson::Document request;
        request.SetObject();
        auto& alloc = request.GetAllocator();
        request.AddMember("result_type", static_cast<int>(result_type), alloc);
        std::string expr_with_semicolon = expression;
        expr_with_semicolon += ";";
        request.AddMember("expression", rapidjson::Value(expr_with_semicolon.c_str(), alloc), alloc);
        rapidjson::Value id(rapidjson::kArrayType);
        for (int i : logical_id)
            id.PushBack(i, alloc);
        request.AddMember("id", id, alloc);
        request.AddMember("timestamp", 1, alloc);
        return request;
    };

    auto has_dependency =
        [](const rapidjson::Document& reply, const char* name)
    {
        if (!reply.HasMember("dependencies") || !reply["dependencies"].IsArray())
            return false;
        for (rapidjson::SizeType i = 0; i < reply["dependencies"].Size(); ++i)
        {
            if (reply["dependencies"][i].IsString() && std::string(reply["dependencies"][i].GetString()) == name)
                return true;
        }
        return false;
    };

    //define the array and the index variable with distinct logical ids
    rapidjson::Document reply;
    solver->Solve(make_request_with_id(ResultType::ARRAY_REAL, "t=[2,3,4]", {1}), reply);
    ASSERT_FALSE(reply.HasMember("error")) << "Unexpected solver error";
    solver->Solve(make_request_with_id(ResultType::ARRAY_REAL, "n=1", {2}), reply);
    ASSERT_FALSE(reply.HasMember("error")) << "Unexpected solver error";

    //the valid element access carries the dependencies on the array and on the index variable
    solver->Solve(make_request_with_id(ResultType::AUTO, "t{n}", {3}), reply);
    ASSERT_FALSE(reply.HasMember("error")) << "Unexpected solver error";
    ASSERT_TRUE(has_dependency(reply, "t"));
    ASSERT_TRUE(has_dependency(reply, "n")) << "the reply must depend on the subscript variable";

    //an invalid index turns the expression into an error, but the dependencies must stay in the reply
    //so that the editor re-solves the expression when the index variable becomes valid again
    solver->Solve(make_request_with_id(ResultType::ARRAY_REAL, "n=10", {2}), reply);
    ASSERT_FALSE(reply.HasMember("error")) << "Unexpected solver error";

    solver->Solve(make_request_with_id(ResultType::AUTO, "t{n}", {3}), reply);
    ASSERT_TRUE(reply.HasMember("error")) << "the out-of-range index must produce an error";
    ASSERT_TRUE(has_dependency(reply, "t"));
    ASSERT_TRUE(has_dependency(reply, "n")) << "the error reply must keep the subscript variable dependency";
}

}
