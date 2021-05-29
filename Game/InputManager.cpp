#include "InputManager.h"

using namespace Tk;

InputManager g_InputManager;

void InputManager::BindKeycodeCallback_KeyDown(uint32 keycode, keycode_callback_func callback)
{
    TINKER_ASSERT(keycode < Tk::Platform::Keycode::eMax); // valid keycode
    TINKER_ASSERT(callback); // non-null function pointer
    m_callbacks_keyDown[keycode] = callback;
}

void InputManager::BindKeycodeCallback_KeyDownRepeat(uint32 keycode, keycode_callback_func callback)
{
    TINKER_ASSERT(keycode < Tk::Platform::Keycode::eMax); // valid keycode
    TINKER_ASSERT(callback); // non-null function pointer
    m_callbacks_keyDownRepeat[keycode] = callback;
}

void InputManager::UpdateAndDoCallbacks(const Platform::InputStateDeltas* inputStateDeltas, const Platform::PlatformAPIFuncs* platformFuncs)
{
    m_previousInputState = m_currentInputState;

    // Apply deltas to input state
    for (uint32 uiKeycode = 0; uiKeycode < Platform::Keycode::eMax; ++uiKeycode)
    {
        if (inputStateDeltas->keyCodes[uiKeycode].numStateChanges > 0)
        {
            m_currentInputState.keyCodes[uiKeycode].isDown = inputStateDeltas->keyCodes[uiKeycode].isDown;
            m_currentInputState.keyCodes[uiKeycode].numStateChanges += inputStateDeltas->keyCodes[uiKeycode].numStateChanges;
            // TODO: reset number of state changes at some point, every second or every several frames or something?
        }
    }
    
    // Process current state
    for (uint32 uiKeycode = 0; uiKeycode < Platform::Keycode::eMax; ++uiKeycode)
    {
        // Initial downpress
        if (m_currentInputState.keyCodes[uiKeycode].isDown && !m_previousInputState.keyCodes[uiKeycode].isDown)
        {
            keycode_callback_func* cbFunc = m_callbacks_keyDown[uiKeycode];
            if (cbFunc)
            {
                cbFunc(platformFuncs);
            }
        }
        else
        {
            // As long as the key is down
            if (m_currentInputState.keyCodes[uiKeycode].isDown)
            {
                keycode_callback_func* cbFunc = m_callbacks_keyDownRepeat[uiKeycode];
                if (cbFunc)
                {
                    cbFunc(platformFuncs);
                }
            }
        }
        // TODO: key up callbacks
    }
}
