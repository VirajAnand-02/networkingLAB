// filepath: e:\programming\NetworkingLAB\terminal-chat-app\server\TCP_server_WIN.c
// tcp_server.c
// gcc TCP_server_WIN.c -o server.exe -lws2_32; .\server.exe
#include <stdio.h>
#include <winsock2.h>
#include <ctype.h>

#pragma comment(lib, "ws2_32.lib")  // Link with ws2_32.lib

char* capitalize(char* text) {
    if (text == NULL) return NULL;

    char* p = text;
    while (*p) {
        *p = toupper(*p);
        p++;
    }
    return text;
}

int main() {
    WSADATA wsa;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server, client;
    int c;
    char* message = "Hello Client!";
    int port;

    // Prompt for port number
    printf("Enter port number to host on: ");
    scanf("%d", &port);

    // Initialize Winsock
    WSAStartup(MAKEWORD(2, 2), &wsa);

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        printf("Socket failed: %d\n", WSAGetLastError());
        return 1;
    }

    // Prepare sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    // Bind
    if (bind(server_socket, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        printf("Bind failed: %d\n", WSAGetLastError());
        return 1;
    }

    listen(server_socket, 3);
    printf("Server is listening on port %d...\n", port);

    c = sizeof(struct sockaddr_in);
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client, &c);
        if (client_socket == INVALID_SOCKET) {
            printf("Accept failed: %d\n", WSAGetLastError());
            continue;
        }

        char client_msg[1024];  // Buffer for receiving
        int recv_len = recv(client_socket, client_msg, sizeof(client_msg) - 1, 0);
        if (recv_len == SOCKET_ERROR) {
            printf("recv failed: %d\n", WSAGetLastError());
        } else {
            client_msg[recv_len] = '\0';  // Null-terminate the string
            
            printf("Received from client: %s\n", client_msg);
            if (strcmp(client_msg, "exit") == 0) {
                printf("Client disconnected.\n");
                closesocket(client_socket);
                break; // Exit the loop if client sends "exit"
            }
        }

        printf("Connection accepted.\n");
        capitalize(client_msg);
        send(client_socket, client_msg, strlen(client_msg), 0);
        printf("Message sent to client.\n");
        closesocket(client_socket); // Close the client socket after responding
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}