#pragma once
#include <string>
#include <filesystem>

namespace utils
{
	std::wstring GetFullResourcePath(const std::wstring& resource)
	{
		// Get buildPath
		wchar_t exePath[MAX_PATH];
		GetModuleFileName(NULL, exePath, MAX_PATH);
		const auto buildPath{ std::filesystem::path(exePath).parent_path() };

		// Return full path
		return buildPath.wstring() + L"/" + resource;
	}
}