#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "config.h"
#include "logger.h"
#include "service_context.h"
#include "session.h"
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>

using namespace yutovo_service;

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

using stream = websocket::stream<typename beast::tcp_stream::rebind_executor<typename net::use_awaitable_t<>::executor_with_default<net::any_io_executor>>::other>;

net::awaitable<void> DoSession(stream ws, ServiceContext* service_context, Logger* logger)
{
    ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
    ws.set_option(websocket::stream_base::decorator(
        [](websocket::response_type& res)
        {
            res.set(http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-server-coro");
        }));

    boost::beast::error_code ec;
    auto remote = ws.next_layer().socket().remote_endpoint(ec);
    std::string remote_str = remote.address().to_string() + ":" + std::to_string(remote.port());

    co_await ws.async_accept();
    if (!ws.is_open())
    {
        logger->Error("Error accepting with {}", remote_str);
        co_return;
    }

    logger->Info("New connection accepted with {}", remote_str);

    beast::flat_buffer buffer;

    Session session(service_context, logger);

    while (true)
    {
        try
        {
            buffer.clear();
            co_await ws.async_read(buffer);

            if (!ws.got_text())
                continue;

            std::string json(boost::asio::buffers_begin(buffer.data()), boost::asio::buffers_end(buffer.data()));
            logger->Info("Request received:\n{}", json);

            std::string reply;
            session.Parse(json, reply);

            ws.text(ws.got_text());

            co_await ws.async_write(boost::asio::buffer(reply));
            logger->Info("Reply sent:\n{}", reply);
        }
        catch (boost::system::system_error& err)
        {
            if (err.code() != websocket::error::closed)
                throw;
            co_return;
        }
    }
}

net::awaitable<void> DoListen(tcp::endpoint end_point, ServiceContext* service_context, Logger* logger)
{
    auto acceptor = net::use_awaitable.as_default_on(tcp::acceptor(co_await net::this_coro::executor));
    acceptor.open(end_point.protocol());
    acceptor.set_option(net::socket_base::reuse_address(true));

    acceptor.bind(end_point);

    acceptor.listen(net::socket_base::max_listen_connections);

    while (true)
    {
        boost::asio::co_spawn(acceptor.get_executor(), DoSession(stream(co_await acceptor.async_accept()), service_context, logger), 
            [logger](std::exception_ptr ex)
            {
                try
                {
                    std::rethrow_exception(ex);
                }
                catch (std::exception& e)
                {
                    logger->Error("Error in session: {}", e.what());
                }
            });
    }
}

int main(int argc, char *argv[])
{
    Logger* logger = Logger::GetInstance(std::string(std::getenv("YUTOVO_DEPLOY")) + "/log", "solver", true, true);
    logger->Info("Yutovo service start");

    Config config(logger);
    config.Read();

    ServiceContext service_context(&config);

    auto const address = net::ip::make_address("0.0.0.0");
    unsigned short const port = 8010;

    net::io_context ioc{1};

    boost::asio::co_spawn(ioc, DoListen(tcp::endpoint{address, port}, &service_context, logger),
        [logger](std::exception_ptr ex)
        {
            if (ex)
            {
                try
                {
                    std::rethrow_exception(ex);
                }
                catch (std::exception& e)
                {
                    logger->Error("Error: {}", e.what());
                }
            }
        });

    std::vector<std::thread> v;
    v.reserve(config.threads_count - 1);
    for (auto i = config.threads_count - 1; i > 0; --i)
        v.emplace_back(
        [&ioc]
        {
            ioc.run();
        });
    ioc.run();

    logger->Info("Yutovo service finish");
    return 0;   
}
