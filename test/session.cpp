/*
 * Yutovo Solver
 * Copyright (C) 2022-2026 Yutovo developers. All rights reserved.
 * This file is a part of the Yutovo project
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <gtest/gtest.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <yutovo-logger/logger.h>
#include "service_config.h"
#include "service_context.h"
#include "session.h"
#include "types.h"

namespace yutovo_test
{

using namespace yutovo_solver;
using namespace yutovo;

struct SessionTest : public testing::Test
{
    Logger* logger = nullptr;
    ServiceConfig config;
    ServiceContext context;
    Session session;

    SessionTest() :
        logger(Logger::GetInstance("", "test", false, false)),
        config(logger),
        context(&config, "", false, false),
        session(&context, "", false, false)
    {
    }

    rapidjson::Document ParseReply(const std::string& reply)
    {
        rapidjson::Document doc;
        doc.Parse<0>(reply.c_str());
        EXPECT_FALSE(doc.HasParseError());
        return doc;
    }

    static std::string MakeSolveRequest(const char* document_guid, const char* solver_guid, int code_id,
        int result_type, const char* expression)
    {
        rapidjson::Document doc;
        doc.SetObject();
        auto& alloc = doc.GetAllocator();
        doc.AddMember("command", "SOLVE_CODE", alloc);
        doc.AddMember("document_guid", rapidjson::Value(document_guid, alloc), alloc);
        doc.AddMember("solver_guid", rapidjson::Value(solver_guid, alloc), alloc);
        doc.AddMember("code_id", code_id, alloc);
        doc.AddMember("solver_type", static_cast<int>(SolverType::CALCULATOR), alloc);
        doc.AddMember("result_type", result_type, alloc);
        std::string expr = expression;
        expr += ";";
        doc.AddMember("expression", rapidjson::Value(expr.c_str(), alloc), alloc);
        doc.AddMember("id", rapidjson::Value(rapidjson::kArrayType).Move(), alloc);
        doc.AddMember("timestamp", 1, alloc);

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);
        return buffer.GetString();
    }
};

TEST_F(SessionTest, invalid_json_returns_json_error)
{
    std::string reply;
    session.Parse("not a json", reply);
    auto doc = ParseReply(reply);
    ASSERT_TRUE(doc.HasMember("error"));
    ASSERT_EQ(doc["error"].GetInt(), static_cast<int>(ErrorCode::JSON_ERROR));
}

TEST_F(SessionTest, missing_command_returns_no_field_error)
{
    std::string reply;
    session.Parse("{}", reply);
    auto doc = ParseReply(reply);
    ASSERT_TRUE(doc.HasMember("error"));
    ASSERT_EQ(doc["error"].GetInt(), static_cast<int>(ErrorCode::NO_FIELD_ERROR));
}

TEST_F(SessionTest, command_not_string_returns_no_field_error)
{
    std::string reply;
    session.Parse("{\"command\":123}", reply);
    auto doc = ParseReply(reply);
    ASSERT_TRUE(doc.HasMember("error"));
    ASSERT_EQ(doc["error"].GetInt(), static_cast<int>(ErrorCode::NO_FIELD_ERROR));
}

TEST_F(SessionTest, unknown_command_returns_unknown_command)
{
    std::string reply;
    session.Parse("{\"command\":\"UNKNOWN\"}", reply);
    auto doc = ParseReply(reply);
    ASSERT_TRUE(doc.HasMember("error"));
    ASSERT_EQ(doc["error"].GetInt(), static_cast<int>(ErrorCode::UNKNOWN_COMMAND));
}

TEST_F(SessionTest, exit_command_sets_exit_flag_and_returns_ok)
{
    ASSERT_FALSE(context.exit);
    std::string reply;
    session.Parse("{\"command\":\"EXIT\"}", reply);
    ASSERT_TRUE(context.exit);
    auto doc = ParseReply(reply);
    ASSERT_TRUE(doc.HasMember("result"));
    ASSERT_EQ(doc["result"].GetInt(), static_cast<int>(ErrorCode::OK));
}

TEST_F(SessionTest, remove_solver_missing_solver_guid_returns_no_field_error)
{
    std::string reply;
    session.Parse("{\"command\":\"REMOVE_SOLVER\",\"code_id\":1}", reply);
    auto doc = ParseReply(reply);
    ASSERT_TRUE(doc.HasMember("error"));
    ASSERT_EQ(doc["error"].GetInt(), static_cast<int>(ErrorCode::NO_FIELD_ERROR));
}

TEST_F(SessionTest, remove_solver_not_found_returns_solver_error)
{
    std::string reply;
    session.Parse("{\"command\":\"REMOVE_SOLVER\",\"solver_guid\":\"missing\",\"code_id\":1}", reply);
    auto doc = ParseReply(reply);
    ASSERT_TRUE(doc.HasMember("error"));
    ASSERT_EQ(doc["error"].GetInt(), static_cast<int>(ErrorCode::SOLVER_ERROR));
}

TEST_F(SessionTest, solve_code_missing_document_guid_returns_no_field_error)
{
    std::string reply;
    session.Parse("{\"command\":\"SOLVE_CODE\"}", reply);
    auto doc = ParseReply(reply);
    ASSERT_TRUE(doc.HasMember("error"));
    ASSERT_EQ(doc["error"].GetInt(), static_cast<int>(ErrorCode::NO_FIELD_ERROR));
}

TEST_F(SessionTest, solve_code_first_call_returns_restarted_error)
{
    std::string reply;
    session.Parse(MakeSolveRequest("doc_json", "solver_json", 1, static_cast<int>(ResultType::REAL), "1+2"), reply);
    auto doc = ParseReply(reply);
    ASSERT_TRUE(doc.HasMember("error"));
    ASSERT_EQ(doc["error"]["error_code"].GetInt(), static_cast<int>(ErrorCode::SOLVER_RESTARTED_ERROR));
}

}
