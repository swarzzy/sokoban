#pragma once
#include "Platform.h"

namespace AB
{
	struct DateTime;

	u32 DateTimeToString(DateTime* dt, char* buffer, u32 bufferSize);

	// If buffer is big enough returns number of characters written (without null terminator),
	// otherwise returns -1
   	i32 FormatString(char* buffer, u32 bufferSize, const char* format, ...);
	void PrintString(const char* format, ...);
	u32 ToString(char* buffer, u32 bufferSize, u64 value);
	u32 ToString(char* buffer, u32 bufferSize, f64 value, u32 precision);

	void Log(LogLevel level, const char* file, const char* func, u32 line, const char* fmt, ...);
	void LogAssert(LogLevel level, const char* file, const char* func, u32 line, const char* assertStr);
	void LogAssert(LogLevel level, const char* file, const char* func, u32 line, const char* assertStr, const char* fmt, ...);
	void LogAssertV(LogLevel level, const char* file, const char* func, u32 line, const char* assertStr, const char* fmt, va_list* args);
	void CutFilenameFromEnd(char* str, char separator = '\\');

// Returns required size if buffer is to small
	struct GetFilenameFromPathRet
	{
		b32 succeeded;
		u64 written;
	};
	
	GetFilenameFromPathRet GetFilenameFromPath(const char* path, char* buffer, u32 buffer_size);
}
