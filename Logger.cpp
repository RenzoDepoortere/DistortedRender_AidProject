#include "Logger.h"

#include <Windows.h>

void Logger::Log(const std::wstring& message)
{
	const std::wstring finalString{ message + L'\n' };
	OutputDebugString(finalString.c_str());
}