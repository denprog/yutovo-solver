#ifndef __SESSION_H__
#define __SESSION_H__

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/dispatch.hpp>
#include <rapidjson/document.h>
#include "types.h"
#include <yutovo_logger/logger.h>

using tcp = boost::asio::ip::tcp;
namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace http = beast::http;
namespace ssl = boost::asio::ssl;

namespace yutovo_service
{

class ServiceContext;
class Config;

using namespace yutovo;

class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(tcp::socket&& socket, ServiceContext* _service_context, Logger* _logger);
    ~Session();

    void Run();

    void Parse(const std::string& json, std::string& reply);

private:
    void OnRun();
    void OnHandshake(beast::error_code ec);
    void OnAccept(beast::error_code ec);
    void DoRead();
    void OnRead(beast::error_code ec, std::size_t bytes_transferred);
    void OnWrite(beast::error_code ec, std::size_t bytes_transferred);

private:
    void MakeError(const ErrorCode error_code, std::string& reply);
    void MakeOk(std::string& reply);
    void MakeReply(const rapidjson::Document& json, std::string& reply);

private:
    ServiceContext* service_context;

    websocket::stream<beast::ssl_stream<beast::tcp_stream>> ws;
    beast::flat_buffer buffer;
    std::string reply;

    Logger* logger;
    static int sessions_count;
};

class Listener : public std::enable_shared_from_this<Listener>
{
public:
    Listener(ServiceContext* _service_context, tcp::endpoint end_point, Logger* _logger);

    void Run();

private:
    void DoAccept();
    void OnAccept(beast::error_code ec, tcp::socket socket);

private:
    ServiceContext* service_context;
    tcp::acceptor acceptor;
    Logger* logger;
};

}

#endif
