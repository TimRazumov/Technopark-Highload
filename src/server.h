#pragma once

#include "config.h"
#include "file_store.h"

#include <thread>
#include <vector>

#include <boost/asio/io_context.hpp>

class StaticServer final
{
public:
	explicit StaticServer(const config::Settings& settings_) noexcept;
	~StaticServer();

	int Start();
	void Stop();

private:
	const config::Settings settings;
	store::FileStore fileStore;
	boost::asio::io_context ioContext;
	std::vector<std::thread> workers;
};