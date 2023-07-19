#include "config.h"
#include <boost/property_tree/ini_parser.hpp>

namespace yutovo_service
{

Config::Config(Logger* _logger) :
    logger(_logger)
{
}

bool Config::Read()
{
    boost::property_tree::ptree pt;
    try
    {
        boost::property_tree::ini_parser::read_ini(file_name, pt);
    }
    catch (boost::property_tree::ptree_error& ex)
    {
        logger->Info("Error reading {}: {}", file_name, ex.what());
        return false;
    }

    try
    {
        proxy_idle_timeout = pt.get<int>("Proxy.idle_timeout");
        solver_idle_timeout = pt.get<int>("Solver.idle_timeout");
    }
    catch (boost::property_tree::ptree_error& ex)
    {
        logger->Info("Error reading config: {}", ex.what());
        return false;
    }

    return true;
}

}
