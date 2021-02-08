#ifndef CORE_DEFINES_H
#define CORE_DEFINES_H

#include <stdint.h>
typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int8_t   int8;
typedef int16_t  int16;
typedef int32_t  int32;
typedef int64_t  int64;

// TODO: get rid of mem leak tracking eventually
#define DISABLE_MEM_TRACKING
#if !defined(DISABLE_MEM_TRACKING) && defined(_WIN32) && defined(_DEBUG)
#define MEM_TRACKING
#endif

#if defined(MEM_TRACKING) && defined(_WIN32) && defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif
//-----

// Debug printing
#define ENABLE_SHADER_PARSING_LOGGING_ALL

#if defined(ENABLE_SHADER_PARSING_LOGGING_ALL)
#include <stdio.h>
#define ENABLE_SHADER_PARSING_LOGGING_ERRORS
#define ENABLE_SHADER_PARSING_LOGGING_DEBUG
#endif

#ifdef ENABLE_SHADER_PARSING_LOGGING_ERRORS
#define PRINT_ERR(...) printf(__VA_ARGS__);
#else PRINT_ERR(...)
#define 
#endif

#ifdef ENABLE_SHADER_PARSING_LOGGING_DEBUG
#define PRINT_DEBUG(...) printf(__VA_ARGS__);
#else
#define PRINT_DEBUG(...)
#endif
//-----

#define CONSUME_SPIRV_WORD(insnStream) (++(*insnStream))
inline uint32 ReadSpirvWord(const uint32** insnStream)
{
    uint32 word = **insnStream;
    CONSUME_SPIRV_WORD(insnStream);
    return word;
}
#define WORD_COUNT(word) ((word & 0xFFFF0000) >> 16)
#define OPCODE(word) (word & 0x0000FFFF)

#endif
