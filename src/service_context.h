#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include "solver.h"
#include "config.h"
#include <boost/beast/ssl.hpp>
#include <fstream>

namespace ssl = boost::asio::ssl;
namespace asio = boost::asio;

namespace yutovo_service
{

struct ServiceContext
{
    ServiceContext(asio::io_context& _io_context, Config* config) :
        io_context(_io_context),
        solvers(config)
    {
        ssl_context.use_certificate_chain_file("yutovo_service.crt");
        ssl_context.use_private_key_file("yutovo_service.key", ssl::context_base::file_format::pem);
        ssl_context.set_default_verify_paths();
        ssl_context.set_options(boost::asio::ssl::context::default_workarounds |
            boost::asio::ssl::context::no_compression |
            boost::asio::ssl::context::no_sslv2 |
            boost::asio::ssl::context::no_sslv3 |
            boost::asio::ssl::context::no_tlsv1
        );        
    }

    asio::io_context& io_context;
    ssl::context ssl_context{ssl::context::tlsv12};
    Solvers solvers;
    bool exit = false;
};

}

#endif
