#include "pch.h"
#include "Logger.h"

namespace MagpieExtJXL {

void Logger::_Log(MagpieLogLevel logLevel, wil::zstring_view msg, const SourceLocation& location) noexcept {
	assert(!msg.empty());

	_hostLog(
		MagpieLogSourceLocation{ location.FileName(), location.Line(), location.FunctionName() },
		logLevel,
		msg.c_str()
	);
}

}
