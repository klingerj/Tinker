#include "SpirvVM.h"

#include <stdio.h>
#include <stdlib.h>

int main()
{
    const char* spvFilePath = "../Shaders/spv/basic_frag_glsl.spv";
    printf("SPV file specified: %s\n", spvFilePath);

    FILE* spvFP;
    errno_t err = fopen_s(&spvFP, spvFilePath, "rb");
    if (err)
    {
        printf("Failed to open file\n");
        return 1;
    }

    fseek(spvFP, 0L, SEEK_END);
    uint32 fileSizeInBytes = ftell(spvFP); // get file size
    fseek(spvFP, 0L, SEEK_SET);
    uint8* fileBuffer = (uint8*)malloc(fileSizeInBytes);
    fread(fileBuffer, 1, fileSizeInBytes, spvFP);
    fclose(spvFP);

    // Run some VM stuff
    {
        //const uint32 numThreads = 1;

        VM_Context* context = CreateContext();
        VM_Shader* shader = CreateShader(context, (uint32*)fileBuffer, fileSizeInBytes);
        VM_State* state = CreateState(context, shader);

        // Pretend interpolated triangle data for fragment shader
        float inNormal[3];
        inNormal[0] = 1.0f;
        inNormal[1] = 0.0f;
        inNormal[2] = 0.0f;

        float inUV[2];
        inUV[0] = 0.5f;
        inUV[1] = 0.5f;

        AddStateInputData(context, state, 0, inUV, sizeof(float) * 2);
        AddStateInputData(context, state, 1, inNormal, sizeof(float) * 3);
        uint8 error = CallEntryPointByName(context, state, "main");

        if (!error)
        {
            printf("Executed the shader.\n");
        }
        else
        {
            // TODO: handle error
        }


        /*for (uint32 i = 0; i < numThreads; ++i)
        {
            state[i].Init(&shader);
            descs[i].Init();
        }

        printf("Executing shader...\n\n");
        for (uint32 i = 0; i < numThreads; ++i)
        {
            SPIRV_VM::ExecuteEntireShader(&state[i], &descs[i], &shader);

            printf("VM state after execution: \n");
            printf("Instructions executed: %lu\n", state[i].programCounter);
            printf("Reg A: %llu\n", state[i].regA);
            printf("Reg B: %llu\n", state[i].regB);*
        }
        for (uint32 i = 0; i < numThreads; ++i)
        {
            delete state[i].resultIDs;
        }*/

        // TODO: turn this back on when we implement full rewinding
        /*
        // Save final states for checking with stepping forward/backward
        SPIRV_VM::State finalState[numThreads];
        SPIRV_VM::Descriptors finalDescs[numThreads];
        for (uint32 i = 0; i < numThreads; ++i)
        {
            finalState[i] = state[i];
            finalDescs[i] = descs[i];
        }

        // Now test state logging, shader stepping and restoring previous vm state

        // Track each state after each insn
        SPIRV_VM::StateLog* stateLog = new SPIRV_VM::StateLog[numThreads];
        SPIRV_VM::DescLog* descLog = new SPIRV_VM::DescLog[numThreads];

        for (uint32 i = 0; i < numThreads; ++i)
        {
            state[i].Init();
            descs[i].Init();
            stateLog[i].Init();
            descLog[i].Init();
        }

        for (uint32 i = 0; i < numThreads; ++i)
        {
            // Step forward
            // TODO: NEED A REAL WAY TO CHECK IF THE SHADER IS STILL NOT FINISHED RUNNING
            for (uint32 uiInsn = 0; uiInsn < 64; ++uiInsn)
            {
                // Step from the beginning all the way up to the current insn
                for (uint32 uiStep = 0; uiStep <= uiInsn; ++uiStep)
                {
                    // Save last state
                    SPIRV_VM::SaveStateInLog(&stateLog[i], &state[i]);
                    SPIRV_VM::SaveStateInLog(&descLog[i], &descs[i]);

                    SPIRV_VM::StepForwardShader(&state[i], &descs[i], &shader);
                }

                // After executing, step back all the way to the initial state
                for (int32 uiLastState = uiInsn; uiLastState >= 0; --uiLastState)
                {
                    // Restore previous vm state
                    SPIRV_VM::RestoreLastStateFromLog(&stateLog[i], &state[i]);

                    // Restore previous descriptor state as well, e.g. if we wrote to a UAV
                    SPIRV_VM::RestoreLastStateFromLog(&descLog[i], &descs[i]);
                }
            }
        }

        // Verify that the final state matches the old saved final state
        // indicating that state restoration worked
        for (uint32 i = 0; i < numThreads; ++i)
        {
            SPIRV_VM::ExecuteEntireShader(&state[i], &descs[i], &shader);
            if (state[i] != finalState[i] ||
                descs[i] != finalDescs[i])
            {
                printf("Rewinding didn't work.\n");
            }
            else
            {
                printf("Rewinding worked.\n");
            }
        }


        delete stateLog;
        delete descLog;
        */
        DestroyState(context, state);
        DestroyShader(context, shader);
    }

    free(fileBuffer);

    return 0;
}
