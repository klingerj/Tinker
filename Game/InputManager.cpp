#include "InputManager.h"

using namespace Tk;

InputManager g_InputManager;

void InputManager::RegisterKeycodeCallback(uint32 keycode, keycode_callback_func callback)
{
    TINKER_ASSERT(keycode < Tk::Platform::Keycode::eMax); // valid keycode
    TINKER_ASSERT(callback); // non-null function pointer
    TINKER_ASSERT(m_callbackCounts[keycode] < (uint32)eMaxCallbacksPerKey); // haven't hit max number of callbacks yet

    m_callbacks[keycode][m_callbackCounts[keycode]++] = callback;
}

void InputManager::CallAllCallbacksForKeycode(uint32 keycode, const Platform::PlatformAPIFuncs* platformFuncs)
{
    for (uint8 i = 0; i < m_callbackCounts[keycode]; ++i)
    {
        m_callbacks[keycode][i](platformFuncs);
    }
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
        if (m_currentInputState.keyCodes[uiKeycode].isDown && !m_previousInputState.keyCodes[uiKeycode].isDown)
        {
            CallAllCallbacksForKeycode(uiKeycode, platformFuncs);
        }

        /* TODO:
        // Handle as long as the key is down
        if (m_currentInputState.keyCodes[uiKeycode].isDown)
        {
        }
        */
    }
}
