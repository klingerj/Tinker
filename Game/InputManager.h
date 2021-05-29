#pragma once

#include "PlatformGameInputHandlingAPI.h"

namespace Tk
{
    namespace Platform
    {
        struct PlatformAPIFuncs;
    }
}

#define KEYCODE_CALLBACK(name) void name(const Tk::Platform::PlatformAPIFuncs* platformFuncs)
typedef KEYCODE_CALLBACK(keycode_callback_func);

struct InputManager
{
private:
    enum { eMaxCallbacksPerKey = 4 };

    typedef struct input_state
    {
        Tk::Platform::KeycodeState keyCodes[Tk::Platform::Keycode::eMax];
    } InputState;
    InputState m_currentInputState = {};
    InputState m_previousInputState = {};
    
    keycode_callback_func* m_callbacks[Tk::Platform::Keycode::eMax][eMaxCallbacksPerKey] = {};
    uint8 m_callbackCounts[Tk::Platform::Keycode::eMax] = {};
    // TODO: differentiate between stuff that should get called back on initial downpress vs held down / repeated

    void CallAllCallbacksForKeycode(uint32 keycode, const Tk::Platform::PlatformAPIFuncs* platformFuncs);

public:
    void RegisterKeycodeCallback(uint32 keycode, keycode_callback_func callback);
    void UpdateAndDoCallbacks(const Tk::Platform::InputStateDeltas* inputStateDeltas, const Tk::Platform::PlatformAPIFuncs* platformFuncs);
};

extern InputManager g_InputManager;
