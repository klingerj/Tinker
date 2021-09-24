#pragma once

#include "Core/CoreDefines.h"
#include "PlatformGameThreadAPI.h"
#include "PlatformGameGraphicsAPI.h"
#include "PlatformGameInputHandlingAPI.h"

namespace Tk
{

namespace Platform
{

// I/O
void PrintDebugString(const char* str);

// Memory
void* AllocAligned(size_t size, size_t alignment, const char* filename, int lineNum);
void FreeAligned(void* ptr);

// Platform File API function
#define READ_ENTIRE_FILE(name) void name(const char* filename, uint32 fileSizeInBytes, uint8* buffer)
typedef READ_ENTIRE_FILE(read_entire_file);

#define WRITE_ENTIRE_FILE(name) void name(const char* filename, uint32 fileSizeInBytes, uint8* buffer)
typedef WRITE_ENTIRE_FILE(write_entire_file);

#define GET_FILE_SIZE(name) uint32 name(const char* filename)
typedef GET_FILE_SIZE(get_file_size);

// Networking
#define INIT_NETWORK_CONNECTION(name) int name()
typedef INIT_NETWORK_CONNECTION(init_network_connection);

#define END_NETWORK_CONNECTION(name) int name()
typedef END_NETWORK_CONNECTION(end_network_connection);

#define SEND_MESSAGE_TO_SERVER(name) int name()
typedef SEND_MESSAGE_TO_SERVER(send_message_to_server);

#define SYSTEM_COMMAND(name) int name(const char* command)
typedef SYSTEM_COMMAND(system_command);

// Platform api functions passed from platform layer to game
struct PlatformAPIFuncs
{
    enqueue_worker_thread_job* EnqueueWorkerThreadJob;
    read_entire_file* ReadEntireFile;
    write_entire_file* WriteEntireFile;
    get_file_size* GetFileSize;
    system_command* SystemCommand;
    init_network_connection* InitNetworkConnection;
    end_network_connection* EndNetworkConnection;
    send_message_to_server* SendMessageToServer;
};

// Game side
#define GAME_UPDATE(name) uint32 name(const Tk::Platform::PlatformAPIFuncs* platformFuncs, Tk::Platform::Graphics::GraphicsCommandStream* graphicsCommandStream, uint32 windowWidth, uint32 windowHeight, const Tk::Platform::InputStateDeltas* inputStateDeltas)
typedef GAME_UPDATE(game_update);

#define GAME_DESTROY(name) void name(const Tk::Platform::PlatformAPIFuncs* platformFuncs)
typedef GAME_DESTROY(game_destroy);

#define GAME_WINDOW_RESIZE(name) void name(Tk::Platform::PlatformAPIFuncs* platformFuncs, uint32 newWindowWidth, uint32 newWindowHeight)
typedef GAME_WINDOW_RESIZE(game_window_resize);

}
}
