#include "pch.h"
#include "Logger.h"

namespace MagpieExtJXL {

void Logger::_Log(MagpieLogLevel logLevel, wil::zstring_view msg, const SourceLocation& location) noexcept {
	assert(!msg.empty());

	_hostLog(
		MagpieLogSourceLocation{ location.GetFileName(), (int)location.GetLine(), location.GetFunctionName() },
		logLevel,
		msg.c_str()
	);
}

static std::string MakeWin32ErrorMsg(std::string_view msg) noexcept {
	return fmt::format("{}\n\tLastErrorCode: {}", msg, GetLastError());
}

void Logger::Info(wil::zstring_view msg, const SourceLocation& location) noexcept {
	_Log(MagpieLogLevelInfo, msg, location);
}

void Logger::Win32Info(wil::zstring_view msg, const SourceLocation& location) noexcept {
	_Log(MagpieLogLevelInfo, MakeWin32ErrorMsg(msg), location);
}

void Logger::Warn(wil::zstring_view msg, const SourceLocation& location) noexcept {
	_Log(MagpieLogLevelWarn, msg, location);
}

void Logger::Win32Warn(wil::zstring_view msg, const SourceLocation& location) noexcept {
	_Log(MagpieLogLevelWarn, MakeWin32ErrorMsg(msg), location);
}

void Logger::Error(wil::zstring_view msg, const SourceLocation& location) noexcept {
	_Log(MagpieLogLevelError, msg, location);
}

void Logger::Win32Error(wil::zstring_view msg, const SourceLocation& location) noexcept {
	_Log(MagpieLogLevelError, MakeWin32ErrorMsg(msg), location);
}

}
