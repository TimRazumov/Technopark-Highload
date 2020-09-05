#include "server.h"

StaticServer::StaticServer(const config::Settings& settings_) noexcept
	: settings(settings_) 
{}

int StaticServer::Run()
{
	BOOST_LOG_TRIVIAL(info) << "run stat server with options port: " << settings.port << " num worker threads: " << settings.numWorkerThreads;
	return EXIT_SUCCESS;
}