#pragma once

#include <string>

#include <boost/log/trivial.hpp>

namespace config 
{

std::string parseCommandLineForConfigFile(int argc, char *argv[]);
void setLogLevel(const std::string &logLevelStr);

struct Settings
{
	explicit Settings(const std::string& configFileName) noexcept;

	size_t port;
	size_t numWorkerThreads;
};

}