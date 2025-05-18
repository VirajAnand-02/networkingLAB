// filepath: terminal-chat-app/client/TCP_client_WIN.c
// tcp_client.c
// gcc TCP_client_WIN.c -o client.exe -lws2_32; .\client.exe
#include <stdio.h>
#include <winsock2.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")  // Link with ws2_32.lib

int main() {
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in server;
    char buffer[1024] = {0};
    char ip[16];
    int port;
    char user_id[50];

    // Prompt for server IP address and port
    printf("Enter server IP address: ");
    fgets(ip, sizeof(ip), stdin);
    ip[strcspn(ip, "\n")] = 0; // Remove newline character

    printf("Enter server port: ");
    scanf("%d", &port);
    getchar(); // Consume newline character left by scanf

    printf("Enter your user id: ");
    fgets(user_id, sizeof(user_id), stdin);
    user_id[strcspn(user_id, "\n")] = 0; // Remove newline character

    // Initialize Winsock
    WSAStartup(MAKEWORD(2, 2), &wsa);

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("Socket failed: %d\n", WSAGetLastError());
        return 1;
    }

    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    // Connect to server
    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        printf("Connect failed: %d\n", WSAGetLastError());
        return 1;
    }

    printf("Connected to server as %s.\n", user_id);

    // send user id
    send(sock, user_id, strlen(user_id), 0);

    while (1) {
        // Allocate memory for the message
        char* msg = (char*)malloc(1024 * sizeof(char));
        if (msg == NULL) {
            printf("Memory allocation failed\n");
            closesocket(sock);
            WSACleanup();
            return 1;
        }

        printf("Enter a message (type 'exit' to disconnect): ");
        fgets(msg, 1024, stdin);

        // Remove the trailing newline character if it exists
        size_t len = strlen(msg);
        if (len > 0 && msg[len-1] == '\n') {
            msg[len-1] = '\0';
        }

        // Check for exit command
        if (strcmp(msg, "exit") == 0) {
            free(msg);
            break;
        }

        // Send message to server
        send(sock, msg, strlen(msg), 0);

        // Receive data
        int recv_len = recv(sock, buffer, sizeof(buffer), 0);
        if (recv_len == SOCKET_ERROR) {
            printf("recv failed: %d\n", WSAGetLastError());
        } else {
            buffer[recv_len] = '\0';
            printf("Received from server: %s\n", buffer);
        }

        free(msg);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}