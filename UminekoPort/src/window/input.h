#pragma once

#include <map>

#include <GLFW/glfw3.h>

enum class KeyCode {
	Space = 32,

	Num0 = 48,
	Num1 = 49,
	Num2 = 50,
	Num3 = 51,
	Num4 = 52,
	Num5 = 53,
	Num6 = 54,
	Num7 = 55,
	Num8 = 56,
	Num9 = 57,

	A = 65, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

	Return = 257, Tab, Backspace, Insert, Delete,

	Right = 262, Left, Down, Up,

	F1 = 290, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25,

	Keypad0 = 320, Keypad1, Keypad2, Keypad3, Keypad4, Keypad5, Keypad6, Keypad7, Keypad8, Keypad9, KeypadDecimal, KeypadDivide, KeypadMultiply, KeypadSubtract, KeypadAdd, KeypadEnter, KeypadEqual,

	LeftShift = 340, LeftControl, LeftAlt, LeftSuper, RightShift, RightControl, RightAlt, RightSuper
};

typedef int KeyState;

enum class MouseButton {
	Left = 0,
	Right = 1,
	Middle = 2,
	Button3,
	Button4,
	Button5,
	Button6
};

class Input {
public:
	static bool keyDown(KeyCode key) {
		return keys_[key] == 1;
	}

	static bool key(KeyCode key) {
		return keys_[key] == 2;
	}

	static bool keyUp(KeyCode key) {
		return keys_[key] == 3;
	}

	static bool mouseDown(MouseButton button) {
		return mouseButtons_[button] == 1;
	}

	static void advanceState() {
		for (auto &key : keys_) {
			if (key.second == 3) {
				key.second = 0;
			}
			++key.second;
		}
		for (auto &button : mouseButtons_) {
			if (button.second == 3) {
				button.second = 0;
			}
			++button.second;
		}
	}
private:
	friend class Window;

	static void setKey(KeyCode key, KeyState state) {
		keys_[key] = state;
	}

	static void setButton(MouseButton button, KeyState state) {
		mouseButtons_[button] = state;
	}

	static std::map<KeyCode, KeyState> keys_;
	static std::map<MouseButton, KeyState> mouseButtons_;
};