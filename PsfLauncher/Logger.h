#pragma once
#include <string>
#include <assert.h>

void Log(const char fmt[], ...) noexcept
{
	std::string str;
	str.resize(256);

	va_list args;
	va_start(args, fmt);
	std::size_t count = std::vsnprintf(str.data(), str.size() + 1, fmt, args);
	assert(count >= 0);
	va_end(args);

	if (count > str.size())
	{
		str.resize(count);

		va_list args2;
		va_start(args2, fmt);
		count = std::vsnprintf(str.data(), str.size() + 1, fmt, args2);
		assert(count >= 0);
		va_end(args2);
	}

	str.resize(count);
	::OutputDebugStringA(str.c_str());
}
void LogString(const char name[], const char value[]) noexcept
{
	Log("\t%s=%s\n", name, value);
}
void LogString(const char name[], const wchar_t value[]) noexcept
{
	Log("\t%s=%ls\n", name, value);
}