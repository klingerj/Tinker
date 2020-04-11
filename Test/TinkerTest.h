#pragma once

#include "../Include/System/SystemDefines.h"

#include <iostream>

extern uint8 g_AssertFailedFlag;

#define MAX_TEST_NAME_CHARS 50

#define TINKER_TEST_ASSERT(cond) \
            g_AssertFailedFlag = !(cond); \
            if (g_AssertFailedFlag) \
            { \
            std::cout << "\nAssert failed at " << __func__ << ":" << __LINE__ << "\n\n"; \
            return; \
            }

#define TINKER_TEST_PRINT_HEADER std::cout << "Tinker Engine Unit Tests\n";
#define TINKER_TEST_PRINT_NAME(name) std::cout << "\n***** " << name << " *****\n";

#define TINKER_TEST(str, func) \
            std::cout << str << " "; \
            for (uint32 i = (uint32)strlen(str); i < MAX_TEST_NAME_CHARS; ++i) std::cout << "."; \
            func(); \
            if (!g_AssertFailedFlag) std::cout << "Passed" << "\n"; \
            else g_AssertFailedFlag = false;
