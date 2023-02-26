#include "pch.h"

#define SPDLOG_USE_STD_FORMAT

#include "Log.h"

#include <filesystem>
#include <iostream>

#include "spdlog/sinks/stdout_color_sinks.h"

namespace Util
{
	std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

	void Log::Init()
	{
		#ifdef ML_LOGGING
		AllocConsole();
		#endif	
		spdlog::set_pattern("%^[%T] %n: %v%$");

		s_ClientLogger = spdlog::stdout_color_mt("ML");
		s_ClientLogger->set_level(spdlog::level::trace);
	}

	void Log::Shutdown()
	{
		s_ClientLogger.reset();
		spdlog::drop_all();
	}
}