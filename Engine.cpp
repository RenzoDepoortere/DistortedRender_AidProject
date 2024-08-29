#include "framework.h"
#include "Engine.h"

#include "InputManager.h"
#include "Renderer.h"

#include <thread>

// Globals
std::unique_ptr<Engine> g_pEngine;

// Windows Functions
// -------------------
#pragma region Windows

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ LPWSTR /*lpCmdLine*/, _In_ int nCmdShow)
{
    g_pEngine = std::make_unique<Engine>(hInstance, nCmdShow);
    return g_pEngine->Run();
}
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return g_pEngine->HandleEvent(hWnd, message, wParam, lParam);
}

#pragma endregion
// -------------------

// Engine
// ------------------
#pragma region Engine

Engine::Engine(HINSTANCE hInstance, int nCmdShow)
    : m_WindowInstance { hInstance }
    , m_InitialWindowMode{ nCmdShow }

    , m_AppName{ L"Graphics Engine" }
    , m_TitleName{ L"Graphics Engine" }

    , m_LastTime{}
    , m_Lag{}

    , m_pInputManager{}
    , m_pRenderer{}
{
    // Set FPS
    const float maxFPS{ 144.f };
    m_MsPerFrame = 1000.f / maxFPS;

    // Create window info
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GRAPHICSENGINE));  // Window icon
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);                          // Window cursor
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);                        // Background color
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = m_AppName.c_str();
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));    // Small window icon

    RegisterClassExW(&wcex);
}
int Engine::Run()
{
    const int windowSize = 640;

    // Create window
    HWND hWnd = CreateWindowW(
        m_AppName.c_str(),      // App Name
        m_TitleName.c_str(),    // Title Name
        WS_OVERLAPPEDWINDOW,    // Window style
        CW_USEDEFAULT,          // Default X
        CW_USEDEFAULT,          // Default Y
        windowSize,             // Default Width
        windowSize,             // Default Height
        nullptr,                // No parent window
        nullptr,                // No menu
        m_WindowInstance,       // Instance handle
        nullptr);               // No additional parameters

    // Faulty creation
    if (!hWnd)
    {
        MessageBox(NULL, L"Failure to create program window", L"Error", MB_ICONERROR);
        return FALSE;
    }

    // Show and update window
    ShowWindow(hWnd, m_InitialWindowMode);
    UpdateWindow(hWnd);

    // Initialize
    // ----------

    // Messages
    MSG message;
    PeekMessage(&message, NULL, 0U, 0U, PM_NOREMOVE);

    // Time
    m_LastTime = std::chrono::high_resolution_clock::now();

    // Input
    m_pInputManager = InputManager::GetInstance();

    // Renderer
    m_pRenderer = std::make_unique<Renderer>(hWnd);
    m_pRenderer->CreateDeviceDependentResources();      // Should be called on scene load
    m_pRenderer->CreateWindowSizeDependentResources();  // Should be called on windowSize change

    // Game loop
    // ---------
    bool messageReceived{ false };
    bool needsToCloseGame{ false };
    while (!needsToCloseGame && WM_QUIT != message.message)
    {
        // Check for message
        messageReceived = PeekMessage(&message, NULL, 0U, 0U, PM_REMOVE) != 0;
        if (messageReceived)
        {
            // Translate and dispatch
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
        // Normal Loop
        else
        {
            needsToCloseGame = GameLoop();
        }
    }

    // Quit application
    return (int) message.wParam;
}
LRESULT Engine::HandleEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        /*HDC hdc = */BeginPaint(hWnd, &ps);
        
        // DRAW

        EndPaint(hWnd, &ps);
    }
    break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

bool Engine::GameLoop()
{
    using namespace std::chrono;

    // Calculate time
    const auto currentTime{ high_resolution_clock::now() };
    const float deltaTime{ duration<float>(currentTime - m_LastTime).count() };
    m_LastTime = currentTime;
    m_Lag += deltaTime;

    // Input
    bool needsToClose = m_pInputManager->HandleInput();

    // FixedUpdate

    // Update

    // Render
    m_pRenderer->Temp_Update(deltaTime);
    m_pRenderer->Render();

    // Sleep
    const auto sleepTime{ currentTime + milliseconds(static_cast<long long>(m_MsPerFrame)) - high_resolution_clock::now() };
    std::this_thread::sleep_for(sleepTime);

    // Return
    return needsToClose;
}

#pragma endregion