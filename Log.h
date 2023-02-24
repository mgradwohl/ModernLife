#pragma once

#include <string>
#include <map>
#include <utility>
#include <source_location>
#include <format>

#define SPDLOG_USE_STD_FORMAT

#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

namespace Util
{
	class Log
	{
	public:
		enum class Level : uint8_t
		{
			Trace = 0, Info, Warn, Error, Fatal
		};
		struct TagDetails
		{
			bool Enabled = true;
			Level LevelFilter = Level::Trace;
		};

	public:
		static void Init();
		static void Shutdown();

		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() noexcept { return s_ClientLogger; }

		static bool HasTag(const std::string& tag) { return s_EnabledTags.find(tag) != s_EnabledTags.end(); }
		static std::map<std::string, TagDetails>& EnabledTags() noexcept { return s_EnabledTags; }

		//template<typename... Args>
		//static void PrintMessage(Log::Level level, std::string_view tag, Args&&... args);

		static void MethodName(const std::source_location& location);

		template<typename... Args>
		static void PrintAssertMessage(std::string_view prefix, Args&&... args);

	public:
		// Enum utils
		static const char* LevelToString(Level level) noexcept
		{
			switch (level)
			{
			case Level::Trace: return "Trace";
			case Level::Info:  return "Info";
			case Level::Warn:  return "Warn";
			case Level::Error: return "Error";
			case Level::Fatal: return "Fatal";
			}
			return "";
		}
		static Level LevelFromString(std::string_view string) noexcept
		{
			if (string == "Trace") return Level::Trace;
			if (string == "Info")  return Level::Info;
			if (string == "Warn")  return Level::Warn;
			if (string == "Error") return Level::Error;
			if (string == "Fatal") return Level::Fatal;

			return Level::Trace;
		}

	private:
		static std::shared_ptr<spdlog::logger> s_ClientLogger;

		inline static std::map<std::string, TagDetails> s_EnabledTags;
	};

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tagged logs (prefer these!)                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Client logging
//#define ML_TRACE_TAG(tag, ...) ::Util::Log::PrintMessage(::Util::Log::Level::Trace, tag, __VA_ARGS__)
//#define ML_INFO_TAG(tag, ...)  ::Util::Log::PrintMessage(::Util::Log::Level::Info, tag, __VA_ARGS__)
//#define ML_WARN_TAG(tag, ...)  ::Util::Log::PrintMessage(::Util::Log::Level::Warn, tag, __VA_ARGS__)
//#define ML_ERROR_TAG(tag, ...) ::Util::Log::PrintMessage(::Util::Log::Level::Error, tag, __VA_ARGS__)
//#define ML_FATAL_TAG(tag, ...) ::Util::Log::PrintMessage(::Util::Log::Level::Fatal, tag, __VA_ARGS__)

// Client Logging
//#define ML_TRACE(...)   ::Util::Log::PrintMessage(::Util::Log::Level::Trace, "", __VA_ARGS__)
//#define ML_INFO(...)    ::Util::Log::PrintMessage(::Util::Log::Level::Info, "", __VA_ARGS__)
//#define ML_WARN(...)    ::Util::Log::PrintMessage(::Util::Log::Level::Warn, "", __VA_ARGS__)
//#define ML_ERROR(...)   ::Util::Log::PrintMessage(::Util::Log::Level::Error, "", __VA_ARGS__)
//#define ML_FATAL(...)   ::Util::Log::PrintMessage(::Util::Log::Level::Fatal, "", __VA_ARGS__)

#ifdef ML_LOGGING
#define ML_METHOD       ::Util::Log::MethodName()
#define ML_TRACE(...)	::Util::Log::GetClientLogger()->trace(__VA_ARGS__)
#define ML_INFO(...)	::Util::Log::GetClientLogger()->info(__VA_ARGS__)
#define ML_WARN(...)	::Util::Log::GetClientLogger()->warn(__VA_ARGS__)
#define ML_ERROR(...)	::Util::Log::GetClientLogger()->error(__VA_ARGS__)
#define ML_FATAL(...)	::Util::Log::GetClientLogger()->critical(__VA_ARGS__)
#else
#define ML_METHOD       
#define ML_TRACE(...)	
#define ML_INFO(...)	
#define ML_WARN(...)	
#define ML_ERROR(...)	
#define HZ_FATAL(...)	
#endif
namespace Util
{
	//template<typename... Args>
	//void Log::PrintMessage(Log::Level level, std::string_view tag, Args&&... args)
	//{
	//	auto detail = s_EnabledTags[std::string(tag)];
	//	if (detail.Enabled && detail.LevelFilter <= level)
	//	{
	//		auto logger = GetClientLogger();
	//		std::string logString = tag.empty() ? "{0}{1}" : "[{0}] {1}";
	//		switch (level)
	//		{
	//		case Level::Trace:
	//			logger->trace(logString, tag, std::format(std::forward<Args>(args)...));
	//			break;
	//		case Level::Info:
	//			logger->info(logString, tag, std::format(std::forward<Args>(args)...));
	//			break;
	//		case Level::Warn:
	//			logger->warn(logString, tag, std::format(std::forward<Args>(args)...));
	//			break;
	//		case Level::Error:
	//			logger->error(logString, tag, std::format(std::forward<Args>(args)...));
	//			break;
	//		case Level::Fatal:
	//			logger->critical(logString, tag, std::format(std::forward<Args>(args)...));
	//			break;
	//		}
	//	}
	//}

	inline void Log::MethodName(const std::source_location& location = std::source_location::current())
	{
		auto logger = GetClientLogger();
		if (logger) logger->info(location.function_name());

	}

	template<typename... Args>
	void Log::PrintAssertMessage(std::string_view prefix, Args&&... args)
	{
		auto logger = GetClientLogger();
		if (logger) logger->error("{0}: {1}", prefix, std::format(std::forward<Args>(args)...));
	}

	template<>
	inline void Log::PrintAssertMessage(std::string_view prefix)
	{
		auto logger = GetClientLogger();
		if (logger) logger->error("{0}", prefix);
	}
}
