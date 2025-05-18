// filepath: e:\programming\NetworkingLAB\terminal-chat-app\server\TCP_server_WIN.c
// tcp_server.c
// gcc TCP_server_WIN.c -o server.exe -lws2_32; .\server.exe
#include <stdio.h>
#include <winsock2.h>
#include <process.h>    // for _beginthreadex
#include <ctype.h>
#include <stdint.h>     // for uintptr_t

#pragma comment(lib, "ws2_32.lib")  // Link with ws2_32.lib

// A simple struct to pass client socket into the thread
typedef struct {
    SOCKET client_socket;
} ClientInfo;

unsigned __stdcall client_thread(void* param) {
    ClientInfo* info = (ClientInfo*)param;
    SOCKET client = info->client_socket;
    free(info);

    char buf[1024];
    int recv_len;

    // Communicate until client sends "exit" or error
    while ((recv_len = recv(client, buf, sizeof(buf)-1, 0)) != SOCKET_ERROR && recv_len > 0) {
        buf[recv_len] = '\0';
        printf("[Thread %u] Received: %s\n", GetCurrentThreadId(), buf);

        if (strcmp(buf, "exit") == 0) {
            printf("[Thread %u] Client requested exit.\n", GetCurrentThreadId());
            break;
        }

        // capitalize in place
        for (int i = 0; buf[i]; i++)
            buf[i] = toupper(buf[i]);

        send(client, buf, strlen(buf), 0);
    }

    closesocket(client);
    printf("[Thread %u] Connection closed.\n", GetCurrentThreadId());
    return 0;
}

int main() {
    WSADATA wsa;
    SOCKET server_socket;
    struct sockaddr_in server;
    int port;

    printf("Enter port number to host on: ");
    scanf("%d", &port);

    WSAStartup(MAKEWORD(2,2), &wsa);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        printf("Socket failed: %d\n", WSAGetLastError());
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        printf("Bind failed: %d\n", WSAGetLastError());
        return 1;
    }

    listen(server_socket, SOMAXCONN);
    printf("Server listening on port %d...\n", port);

    while (1) {
        SOCKET client_socket = accept(server_socket, NULL, NULL);
        if (client_socket == INVALID_SOCKET) {
            printf("Accept failed: %d\n", WSAGetLastError());
            continue;
        }

        // Allocate and fill client info
        ClientInfo* info = malloc(sizeof(*info));
        info->client_socket = client_socket;

        // Spawn a thread to handle the client
        uintptr_t thread_handle = _beginthreadex(
            NULL,               // security
            0,                  // stack size (0 = default)
            client_thread,      // thread function
            info,               // argument
            0,                  // creation flags
            NULL                // thread id
        );
        if (thread_handle == 0) {
            printf("Failed to create thread.\n");
            closesocket(client_socket);
            free(info);
        } else {
            CloseHandle((HANDLE)thread_handle);
        }
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}
