#pragma once
#include "resource.h"

#include <memory>
#include <string>
#include <chrono>

class InputManager;
class Renderer;

class Engine final
{
public:
	// Rule of five
	Engine(HINSTANCE hInstance, int nCmdShow);
	~Engine() = default;

	Engine(const Engine& other) = delete;
	Engine(Engine&& other) = delete;
	Engine& operator= (const Engine& other) = delete;
	Engine& operator= (Engine&& other) = delete;

	// Publics
	int Run();
	LRESULT HandleEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	// Member variables
	HINSTANCE m_WindowInstance;
	int m_InitialWindowMode;

	std::wstring m_AppName;
	std::wstring m_TitleName;

	float m_MsPerFrame;
	std::chrono::steady_clock::time_point m_LastTime;
	float m_Lag;

	InputManager* m_pInputManager;
	std::unique_ptr<Renderer> m_pRenderer;

	// Member functions
	bool GameLoop();
};