#include "config.h"

#include <iostream>

#include <boost/program_options.hpp>

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>

std::string config::parseCommandLineForConfigFile(int argc, char *argv[])
{
	namespace po = boost::program_options;
    try
    {
        po::options_description desc("Allow options");
        std::string configFileName;

        desc.add_options()("help,h", "produce help message")("config,c",
            po::value<std::string>(&configFileName)->required(), "configuration file name");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        if (vm.count("help"))
        {
            std::cout << desc << std::endl;
            exit(EXIT_SUCCESS);
        }
        po::notify(vm);

        return configFileName;
    }
    catch (const po::error &e)
    {
        std::cout << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
}

void config::setLogLevel(const std::string& logLevelStr)
{
	auto logLevel = boost::log::trivial::debug;
    if (logLevelStr == "debug")
    {
    	logLevel = boost::log::trivial::debug;
    }
    else if (logLevelStr == "info")
    {
    	logLevel = boost::log::trivial::info;
    }
    else if (logLevelStr == "warning")
    {
    	logLevel = boost::log::trivial::warning;
    }
    else if (logLevelStr == "error")
    {
    	logLevel = boost::log::trivial::error;
    }
    else if (logLevelStr == "fatal")
    {
    	logLevel = boost::log::trivial::fatal;
    }
    else
    {
    	throw std::runtime_error(
        	"invalid loglevel('" + logLevelStr + "'), valid is debug|info|warning|error|fatal");
	}
	boost::log::core::get()->set_filter(boost::log::trivial::severity >= logLevel);
}

config::Settings::Settings(const std::string& configFileName) noexcept
{
	namespace po = boost::program_options;
    po::options_description desc("Config file");
    std::string logLevel;

    desc.add_options()
        ("port", po::value<short unsigned int>(&port)->required(), "port on which the service is listening")
        ("workers_count", po::value<size_t>(&workersCount)->required(),"num worker threads")
        ("global_path", po::value<std::string>(&globalPath)->required(),"global path to file store")
        ("log_level", po::value<std::string>(&logLevel)->required(), "log level");

    po::variables_map vm;
    po::store(po::parse_config_file<char>(configFileName.data(), desc), vm);
    po::notify(vm);
    config::setLogLevel(logLevel);
}