#pragma once
#include "MagpieExtApi.h"
#include "Singleton.h"

namespace MagpieExtJXL {

// std::source_location 中的函数名包含整个签名过于冗长，我们只需记录函数名，
// 因此创建自己的 SourceLocation。
struct SourceLocation {
	[[nodiscard]] static consteval SourceLocation Current(
		int line = __builtin_LINE(),
		const char* file = __builtin_FILE(),
		const char* function = __builtin_FUNCTION()
	) noexcept {
		return SourceLocation{ line, file, function };
	}

	[[nodiscard]] constexpr SourceLocation() noexcept = default;

	[[nodiscard]] constexpr SourceLocation(
		int line,
		const char* file,
		const char* function
	) noexcept : _line(line), _file(file), _function(function) {}

	[[nodiscard]] constexpr int Line() const noexcept {
		return _line;
	}

	[[nodiscard]] constexpr const char* FileName() const noexcept {
		return _file;
	}

	constexpr const char* FunctionName() const noexcept {
		return _function;
	}

private:
	const int _line = 0;
	const char* _file = nullptr;
	const char* _function = nullptr;
};

class Logger : public Singleton<Logger> {
	friend Singleton<Logger>;

public:
	void Initialize(MagpieHostApi* hostApi) noexcept {
		_hostLog = hostApi->log;
	}

	void Info(wil::zstring_view msg, const SourceLocation& location = SourceLocation::Current()) noexcept {
		_Log(MagpieLogLevelInfo, msg, location);
	}

	void Win32Info(wil::zstring_view msg, const SourceLocation& location = SourceLocation::Current()) noexcept {
		_Log(MagpieLogLevelInfo, _MakeWin32ErrorMsg(msg), location);
	}

	void Warn(wil::zstring_view msg, const SourceLocation& location = SourceLocation::Current()) noexcept {
		_Log(MagpieLogLevelWarn, msg, location);
	}

	void Win32Warn(wil::zstring_view msg, const SourceLocation& location = SourceLocation::Current()) noexcept {
		_Log(MagpieLogLevelWarn, _MakeWin32ErrorMsg(msg), location);
	}

	void Error(wil::zstring_view msg, const SourceLocation& location = SourceLocation::Current()) noexcept {
		_Log(MagpieLogLevelError, msg, location);
	}

	void Win32Error(wil::zstring_view msg, const SourceLocation& location = SourceLocation::Current()) noexcept {
		_Log(MagpieLogLevelError, _MakeWin32ErrorMsg(msg), location);
	}

private:
	Logger() = default;

	static std::string _MakeWin32ErrorMsg(std::string_view msg) noexcept {
		return fmt::format("{}\n\tLastErrorCode: {}", msg, GetLastError());
	}

	void _Log(MagpieLogLevel logLevel, wil::zstring_view msg, const SourceLocation& location) noexcept;

	HRESULT (*_hostLog)(MagpieLogSourceLocation sourceLocation, MagpieLogLevel level, const char* message) = nullptr;
};

}
