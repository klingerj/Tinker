#include "InputManager.h"

using namespace Tk;

InputManager g_InputManager;

void InputManager::RegisterKeycodeCallback(uint32 keycode, keycode_callback_func callback)
{
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
            // TODO: reset number of state changes at some point, every second or every several frames or something
        }
    }

    // Process current state
    for (uint32 uiKeycode = 0; uiKeycode < Platform::Keycode::eMax; ++uiKeycode)
    {
        switch (uiKeycode)
        {
            case Platform::Keycode::eF9:
            {
                // Handle the initial downpress once and nothing more
                if (m_currentInputState.keyCodes[uiKeycode].isDown && !m_previousInputState.keyCodes[uiKeycode].isDown)
                {
                    /*
                    Platform::PrintDebugString("Running raytrace test...\n");
                    RaytraceTest(platformFuncs);
                    Platform::PrintDebugString("...Done.\n");
                    */
                }
                break;
            }

            case Platform::Keycode::eF10:
            {
                // Handle the initial downpress once and nothing more
                if (m_currentInputState.keyCodes[uiKeycode].isDown && !m_previousInputState.keyCodes[uiKeycode].isDown)
                {
                    /*
                    Platform::PrintDebugString("Attempting to hotload shaders...\n");

                    // Recompile shaders via script
                    const char* shaderCompileCommand = SCRIPTS_PATH "build_compile_shaders_glsl2spv.bat";
                    if (platformFuncs->SystemCommand(shaderCompileCommand) != 0)
                    {
                        Platform::PrintDebugString("Failed to create shader compile process! Shaders will not be compiled.\n");
                    }
                    else
                    {
                        // Recreate gpu resources
                        RecreateShaders(platformFuncs, currentWindowWidth, currentWindowHeight);
                        Platform::PrintDebugString("...Done.\n");
                    }
                    */
                }

                // Handle as long as the key is down
                if (m_currentInputState.keyCodes[uiKeycode].isDown)
                {
                }
                
                break;
            }

            default:
            {
                break;
            }
        }
    }
}

