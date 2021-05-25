#include "SpirvVM.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

void WriteBMP(uint8* imageData)
{
    // for now, nabbed from: https://stackoverflow.com/questions/2654480/writing-bmp-image-in-pure-c-c-without-other-libraries
    const int w = 400; /* Put here what ever width you want */
    const int h = 400; /* Put here what ever height you want */
    int* red = (int*)malloc(w * h);
    int* green = (int*)malloc(w * h);
    int* blue = (int*)malloc(w * h);

    FILE* f;
    unsigned char* img = NULL;
    int filesize = 54 + 3 * w * h;  //w is your image width, h is image height, both int
    if (img)
        free(img);
    img = (unsigned char*)malloc(3 * w * h);
    memset(img, 0, 3 * w * h);
    int x;
    int y;

    for (int i = 0; i < w; i++)
    {
        for (int j = 0; j < h; j++)
        {
            x = i; y = (h - 1) - j;
            img[(x + y * w) * 3 + 2] = (unsigned char)(imageData[(i + j * w) * 3 + 0]);
            img[(x + y * w) * 3 + 1] = (unsigned char)(imageData[(i + j * w) * 3 + 1]);
            img[(x + y * w) * 3 + 0] = (unsigned char)(imageData[(i + j * w) * 3 + 2]);
        }
    }

    unsigned char bmpfileheader[14] = { 'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0 };
    unsigned char bmpinfoheader[40] = { 40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0 };
    unsigned char bmppad[3] = { 0,0,0 };

    bmpfileheader[2] = (unsigned char)(filesize);
    bmpfileheader[3] = (unsigned char)(filesize >> 8);
    bmpfileheader[4] = (unsigned char)(filesize >> 16);
    bmpfileheader[5] = (unsigned char)(filesize >> 24);

    bmpinfoheader[4] = (unsigned char)(w);
    bmpinfoheader[5] = (unsigned char)(w >> 8);
    bmpinfoheader[6] = (unsigned char)(w >> 16);
    bmpinfoheader[7] = (unsigned char)(w >> 24);
    bmpinfoheader[8] = (unsigned char)(h);
    bmpinfoheader[9] = (unsigned char)(h >> 8);
    bmpinfoheader[10] = (unsigned char)(h >> 16);
    bmpinfoheader[11] = (unsigned char)(h >> 24);

    fopen_s(&f, "../Output/TestImages/spirv_output.bmp", "wb");
    fwrite(bmpfileheader, 1, 14, f);
    fwrite(bmpinfoheader, 1, 40, f);
    for (int i = 0; i < h; i++)
    {
        fwrite(img + (w * (h - i - 1) * 3), 3, w, f);
        fwrite(bmppad, 1, (4 - (w * 3) % 4) % 4, f);
    }
    fclose(f);
}

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

        // Make a stupid test rasterizer thing

        if (1)
        {
            uint32 width = 400;
            uint32 height = 400;
            uint8* image = (uint8*)malloc(sizeof(uint8) * width * height * 3);
            memset(image, 0, sizeof(uint8) * width * height * 3);
            for (uint32 px = 0; px < width; ++px)
            {
                for (uint32 py = 0; py < height; ++py)
                {
                    float u = (float)px / width;
                    u += 0.5f / width;

                    float v = (float)py / height;
                    v += 0.5f / height;

                    u = u * 2.0f - 1.0f;
                    v = v * 2.0f - 1.0f;

                    float len = sqrtf(u * u + v * v);
                    const float radius = 0.25f;
                    if (len < radius)
                    {
                        u /= radius;
                        v /= radius;

                        float z = sqrtf(1.0f - (u * u + v * v));

                        float inNormal[3];
                        inNormal[0] = u;
                        inNormal[1] = v;
                        inNormal[2] = z;

                        float inUV[2];
                        inUV[0] = 0.5f;
                        inUV[1] = 0.5f;

                        VM_State* state = CreateState(context, shader);

                        AddStateInputData(context, state, 0, inUV, sizeof(float) * 2);
                        AddStateInputData(context, state, 1, inNormal, sizeof(float) * 3);

                        //printf("\n\nExecuting the shader...\n");
                        uint8 error = CallEntryPointByName(context, state, "main");
                        if (!error)
                        {
                            //printf("\n\n...Executed the shader.\n");
                        }
                        else
                        {
                            // TODO: handle error
                        }

                        float* fragColor = (float*)ReadOutputData(state, 0);// (float*)(state->outputData[0])[0]; // get x-component of fragcoord at location 0
                        uint8 grayscaleValue = (uint8)(*fragColor * 255.0f);
                        image[(py * width + px) * 3 + 0] = grayscaleValue;
                        //image[(py * width + px) * 3 + 0] = (uint8)(fabsf(inNormal[0]) * 255.0f);
                        //image[(py * width + px) * 3 + 1] = (uint8)(fabsf(inNormal[1]) * 255.0f);
                        //image[(py * width + px) * 3 + 2] = (uint8)(z * 255.0f);

                        DestroyState(context, state);
                    }
                }
            }

            // Output image as bmp
            //const char* imagePath = "result.bmp";
            WriteBMP(image);
            free(image);
        }
        else
        {
            VM_State* state = CreateState(context, shader);

            float inNormal[3];
            inNormal[0] = 1.0f;
            inNormal[1] = 0.0f;
            inNormal[2] = 0.0f;

            float inUV[2];
            inUV[0] = 0.5f;
            inUV[1] = 0.5f;

            AddStateInputData(context, state, 0, inUV, sizeof(float) * 2);
            AddStateInputData(context, state, 1, inNormal, sizeof(float) * 3);

            printf("\n\nExecuting the shader...\n");
            uint8 error = CallEntryPointByName(context, state, "main");

            if (!error)
            {
                printf("\n\n...Executed the shader.\n");
            }
            else
            {
                // TODO: handle error
            }

            DestroyState(context, state);
        }

        DestroyShader(context, shader);
        DestroyContext(context);


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
    }

    free(fileBuffer);

    return 0;
}
