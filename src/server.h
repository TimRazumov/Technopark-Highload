#pragma once

#include "config.h"

#include <boost/asio.hpp>

class StaticServer final
{
public:
	explicit StaticServer(const config::Settings& settings_) noexcept;
	int Run();

private:
	config::Settings settings;
};