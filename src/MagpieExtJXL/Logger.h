#pragma once
#include "MagpieExtApi.h"
#include "Singleton.h"

namespace MagpieExtJXL {

// std::source_location 中的函数名包含整个签名过于冗长，我们只需记录函数名，
// 因此创建自己的 SourceLocation。
struct SourceLocation {
	[[nodiscard]] static consteval SourceLocation Current(
		std::uint_least32_t line = __builtin_LINE(),
		const char* file = __builtin_FILE(),
		const char* function = __builtin_FUNCTION()
	) noexcept {
		return SourceLocation{ line, file, function };
	}

	[[nodiscard]] constexpr SourceLocation() noexcept = default;

	[[nodiscard]] constexpr SourceLocation(
		std::uint_least32_t line,
		const char* file,
		const char* function
	) noexcept : _line(line), _file(file), _function(function) {}

	[[nodiscard]] constexpr std::uint_least32_t GetLine() const noexcept {
		return _line;
	}

	[[nodiscard]] constexpr const char* GetFileName() const noexcept {
		return _file;
	}

	constexpr const char* GetFunctionName() const noexcept {
		return _function;
	}

private:
	const std::uint_least32_t _line = 0;
	const char* _file = nullptr;
	const char* _function = nullptr;
};

class Logger : public Singleton<Logger> {
	friend Singleton<Logger>;

public:
	void Initialize(MagpieHostApi* hostApi) noexcept {
		_hostLog = hostApi->log;
	}

	void Info(wil::zstring_view msg, const SourceLocation& location = SourceLocation::Current()) noexcept;
	void Win32Info(wil::zstring_view msg, const SourceLocation& location = SourceLocation::Current()) noexcept;

	void Warn(wil::zstring_view msg, const SourceLocation& location = SourceLocation::Current()) noexcept;
	void Win32Warn(wil::zstring_view msg, const SourceLocation& location = SourceLocation::Current()) noexcept;

	void Error(wil::zstring_view msg, const SourceLocation& location = SourceLocation::Current()) noexcept;
	void Win32Error(wil::zstring_view msg, const SourceLocation& location = SourceLocation::Current()) noexcept;

private:
	Logger() = default;

	void _Log(MagpieLogLevel logLevel, wil::zstring_view msg, const SourceLocation& location) noexcept;

	HRESULT (*_hostLog)(MagpieLogSourceLocation sourceLocation, MagpieLogLevel level, const char* message) = nullptr;
};

}
