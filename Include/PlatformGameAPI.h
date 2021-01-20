#pragma once

#include "Core/CoreDefines.h"
#include "PlatformGameThreadAPI.h"
#include "PlatformGameGraphicsAPI.h"
#include "PlatformGameInputHandingAPI.h"

// Quick simple memory allocation and leak tracking
#define MEM_TRACKING

#ifdef MEM_TRACKING
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif

namespace Tinker
{
    namespace Platform
    {
        // I/O
        void PrintDebugString(const char* str);

        // Memory
        void* AllocAligned(size_t size, size_t alignment);
        #ifdef MEM_TRACKING
        void* AllocAligned_Tracked(size_t size, size_t alignment, const char* filename, int lineNum);
        #endif
        void FreeAligned(void* ptr);

        // Platform File API function
        #define READ_ENTIRE_FILE(name) void name(const char* filename, uint32 fileSizeInBytes, uint8* buffer)
        typedef READ_ENTIRE_FILE(read_entire_file);

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
        typedef struct platform_api_functions
        {
            enqueue_worker_thread_job* EnqueueWorkerThreadJob;
            wait_on_thread_job* WaitOnThreadJob;
            read_entire_file* ReadEntireFile;
            get_file_size* GetFileSize;
            init_network_connection* InitNetworkConnection;
            end_network_connection* EndNetworkConnection;
            send_message_to_server* SendMessageToServer;
            system_command* SystemCommand;
            create_resource* CreateResource;
            destroy_resource* DestroyResource;
            map_resource* MapResource;
            unmap_resource* UnmapResource;
            create_framebuffer* CreateFramebuffer;
            destroy_framebuffer* DestroyFramebuffer;
            create_graphics_pipeline* CreateGraphicsPipeline;
            destroy_graphics_pipeline* DestroyGraphicsPipeline;
            create_descriptor* CreateDescriptor;
            destroy_descriptor* DestroyDescriptor;
            destroy_all_descriptors* DestroyAllDescriptors;
            write_descriptor* WriteDescriptor;
            submit_cmds_immediate* SubmitCmdsImmediate;
        } PlatformAPIFuncs;

        // Game side
        #define GAME_UPDATE(name) uint32 name(const Tinker::Platform::PlatformAPIFuncs* platformFuncs, Tinker::Platform::GraphicsCommandStream* graphicsCommandStream, uint32 windowWidth, uint32 windowHeight, const Tinker::Platform::InputStateDeltas* inputStateDeltas)
        typedef GAME_UPDATE(game_update);

        #define GAME_DESTROY(name) void name(Tinker::Platform::PlatformAPIFuncs* platformFuncs)
        typedef GAME_DESTROY(game_destroy);

        #define GAME_WINDOW_RESIZE(name) void name(Tinker::Platform::PlatformAPIFuncs* platformFuncs, uint32 newWindowWidth, uint32 newWindowHeight)
        typedef GAME_WINDOW_RESIZE(game_window_resize);
    }
}
