#pragma once

#include "PlatformGameInputHandlingAPI.h"

#define KEYCODE_CALLBACK(name) void name()
typedef KEYCODE_CALLBACK(keycode_callback_func);

struct InputManager
{
private:
    typedef struct input_state
    {
        Tk::Platform::KeycodeState keyCodes[Platform::Keycode::eMax];
    } InputState;
    InputState m_currentInputState = {};
    InputState m_previousInputState = {};


public:
    void RegisterKeycodeCallback(uint32 keycode, keycode_callback_func callback);
    void UpdateAndDoCallbacks(const Tk::Platform::InputStateDeltas* inputStateDeltas, const Tk::Platform::PlatformAPIFuncs* platformFuncs);
};

extern InputManager g_InputManager;

