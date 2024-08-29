#pragma once
#include "Singleton.h"

#include <array>
#include <Windows.h>

class InputManager final : public Singleton<InputManager>
{
public:
	// Destructor
	virtual ~InputManager() override = default;

	// Publics
	bool HandleInput();

	bool IsKeyPressed(int key);
	bool IsKeyReleased(int key);

private:
	// Initialization
	friend class Singleton<InputManager>;
	InputManager();

	InputManager(const InputManager& other) = delete;
	InputManager(InputManager&& other) = delete;
	InputManager& operator= (const InputManager& other) = delete;
	InputManager& operator= (InputManager&& other) = delete;


	// Member variables
	std::array<BYTE, 256> m_pKeyboardState;
	std::array<BYTE, 256> m_pPrevKeyboardState;
};

