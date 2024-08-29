#pragma once
#include <iostream>

class Logger final
{
public:
	// Rule of five
	~Logger() = default;

	Logger(const Logger& other) = delete;
	Logger(Logger&& other) = delete;
	Logger& operator= (const Logger& other) = delete;
	Logger& operator= (Logger&& other) = delete;

	// Publics
	static void Log(const std::wstring& message);

private:
	// Constructor
	Logger() = default;
};
