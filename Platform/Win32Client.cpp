#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "../Include/Platform/Win32Client.h"
#include "../Include/Platform/Win32Logs.h"
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdlib.h>
#include <string.h>

#define DEFAULT_PORT "8000"

SOCKET ConnectSocket = INVALID_SOCKET;
struct addrinfo* infos;

namespace Tinker
{
    namespace Platform
    {
        namespace Network
        {
            int InitClient()
            {
                // Initialize Winsock
                WSADATA wsaData;
                int result;

                // Initialize Winsock
                result = WSAStartup(MAKEWORD(2,2), &wsaData);
                if (result != 0)
                {
                    char buffer[50] = {};
                    const char* str = "WSAStartup failed with error code: ";
                    strcpy_s(buffer, str);
                    size_t len = strlen(str);
                    _itoa_s(result, buffer + len, 10, 10);
                    buffer[len + 1] = '\0';
                    LogMsg(buffer, eLogSeverityCritical);
                    return 1;
                }

                // Confirm that the Winsock dll is the correct version
                if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
                {
                    LogMsg("Could not find a usable version of Winsock.dll. Exiting.", eLogSeverityCritical);
                    WSACleanup();
                    return 1;
                }
                else
                {
                    LogMsg("The Winsock 2.2 dll was found okay.", eLogSeverityInfo);
                }

                struct addrinfo hints = {};
                hints.ai_family = AF_UNSPEC; // TODO: change?
                hints.ai_socktype = SOCK_STREAM;
                hints.ai_protocol = IPPROTO_TCP;
                
                result = getaddrinfo("10.0.0.33", DEFAULT_PORT, &hints, &infos);
                //result = getaddrinfo("127.0.0.1", DEFAULT_PORT, &hints, &infos);

                if (result != 0)
                {
                    char buffer[50] = {};
                    const char* str = "getaddrinfo failed with error code: ";
                    strcpy_s(buffer, str);
                    size_t len = strlen(str);
                    _itoa_s(result, buffer + len, 10, 10);
                    buffer[len + 1] = '\0';
                    LogMsg(buffer, eLogSeverityCritical);
                    WSACleanup();
                    return 1;
                }

                // Create the connection socket
                // TODO: walk the list of addrinfos to find a valid entry
                ConnectSocket = socket(infos->ai_family, infos->ai_socktype, infos->ai_protocol);
                if (ConnectSocket == INVALID_SOCKET)
                {
                    char buffer[50] = {};
                    const char* str = "socket failed with error code: ";
                    strcpy_s(buffer, str);
                    size_t len = strlen(str);
                    _itoa_s(WSAGetLastError(), buffer + len, 10, 10);
                    buffer[len + 1] = '\0';
                    LogMsg(buffer, eLogSeverityCritical);
                    WSACleanup();
                    freeaddrinfo(infos);
                    return 1;
                }

                // Connect to the server
                LogMsg("Attempting to connect to the server...", eLogSeverityInfo);
                result = connect(ConnectSocket, infos->ai_addr, (int)infos->ai_addrlen);
                if (result == SOCKET_ERROR)
                {
                    closesocket(ConnectSocket);
                    ConnectSocket = INVALID_SOCKET;
                }

                // TODO: try a different entry in the infos list?

                freeaddrinfo(infos);

                if (ConnectSocket == INVALID_SOCKET)
                {
                    LogMsg("connect failed", eLogSeverityCritical);
                    WSACleanup();
                    return 1;
                }

                return 0;
            }
            
            void CleanupClient()
            {
                if (ConnectSocket != INVALID_SOCKET)
                {
                    closesocket(ConnectSocket);
                }
                WSACleanup();
            }

            int DisconnectFromServer()
            {
                LogMsg("Shutting down client.", eLogSeverityInfo);

                // Shutdown receiving from the server
                int result = shutdown(ConnectSocket, SD_RECEIVE);
                if (result == SOCKET_ERROR)
                {
                    char buffer[50] = {};
                    const char* str = "shutdown failed with error code: ";
                    strcpy_s(buffer, str);
                    size_t len = strlen(str);
                    _itoa_s(WSAGetLastError(), buffer + len, 10, 10);
                    buffer[len + 1] = '\0';
                    LogMsg(buffer, eLogSeverityCritical);
                    WSACleanup();
                    return 1;
                }

                // Shutdown sending to the server
                result = shutdown(ConnectSocket, SD_SEND);
                if (result == SOCKET_ERROR)
                {
                    char buffer[50] = {};
                    const char* str = "shutdown failed with error code: ";
                    strcpy_s(buffer, str);
                    size_t len = strlen(str);
                    _itoa_s(WSAGetLastError(), buffer + len, 10, 10);
                    buffer[len + 1] = '\0';
                    LogMsg(buffer, eLogSeverityCritical);
                    WSACleanup();
                    return 1;
                }
                CleanupClient();
                return 0;
            }

            int SendMessageToServer()
            {
                // Send data to the server
                LogMsg("Attempting to send data to the server...", eLogSeverityInfo);
                char clientMsg[512] = {};
                clientMsg[0] = 'J';
                clientMsg[1] = 'o';
                clientMsg[2] = 'e';
                clientMsg[3] = '\n';
                
                int numBytesSent = 0;
                numBytesSent = send(ConnectSocket, clientMsg, 512, 0);
                if (numBytesSent > 0)
                {
                    LogMsg("Successfully sent message to server", eLogSeverityInfo);
                }
                else if (numBytesSent == 0)
                {
                    LogMsg("Received 0 bytes - connection has been gracefully closed.", eLogSeverityInfo);
                }
                else
                {
                    char buffer[50] = {};
                    const char* str = "send failed with error code: ";
                    strcpy_s(buffer, str);
                    size_t len = strlen(str);
                    _itoa_s(WSAGetLastError(), buffer + len, 10, 10);
                    buffer[len + 1] = '\0';
                    LogMsg(buffer, eLogSeverityCritical);
                    DisconnectFromServer();
                    return 1;
                }

                return 0;
            }
        }
    }
}
