#include "InputManager.h"

#include <WinUser.h>

InputManager::InputManager()
	: Singleton()
	, m_pKeyboardState{}
	, m_pPrevKeyboardState{}
{
	GetKeyboardState(m_pKeyboardState.data());
	m_pPrevKeyboardState = m_pKeyboardState;
}

bool InputManager::HandleInput()
{
	// Get keyboardState
	m_pPrevKeyboardState = m_pKeyboardState;
	GetKeyboardState(m_pKeyboardState.data());

	// Check if pressed ESCAPE
	bool pressedEscape = IsKeyPressed(VK_ESCAPE);
	return pressedEscape;
}

bool InputManager::IsKeyPressed(int key)
{
	bool keyIsDown = m_pKeyboardState[key] >> 7;	// Shift the bits, high-order bit is put on the least significant position
	return keyIsDown;
}
bool InputManager::IsKeyReleased(int key)
{
	bool currentlyPressed = m_pKeyboardState[key] >> 7;
	bool wasPressed = m_pPrevKeyboardState[key] >> 7;

	// True if was pressed and currently not pressed
	return wasPressed && !currentlyPressed;
}