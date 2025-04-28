// Compile command: g++ client.cpp -o client.exe -lws2_32
// Run command:   .\client.exe
#include <stdio.h>
#include <winsock2.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib") // Link with ws2_32.lib

#define DEFAULT_PORT 8080 // You might want a default port too
#define DEFAULT_IP "127.0.0.1"
#define BUFFER_SIZE 1024

int main() {
    WSADATA wsa;
    SOCKET sock = INVALID_SOCKET; // Initialize to invalid state
    struct sockaddr_in server;
    char server_ip[16];          // Buffer for IP address input
    int server_port;
    char message[BUFFER_SIZE];   // Buffer for sending messages
    char recv_buffer[BUFFER_SIZE]; // Buffer for receiving messages

    // 1. Prompt for server IP address (with default)
    printf("Enter server IP address (leave blank for %s): ", DEFAULT_IP);
    if (fgets(server_ip, sizeof(server_ip), stdin) != NULL) {
        server_ip[strcspn(server_ip, "\n")] = 0; // Remove trailing newline
        if (server_ip[0] == '\0') { // Check if input is empty
            strcpy(server_ip, DEFAULT_IP);
            printf("Using default IP: %s\n", server_ip);
        }
    } else {
        // Handle potential fgets error (though unlikely for stdin)
        fprintf(stderr, "Error reading IP address.\n");
        return 1;
    }


    // 2. Prompt for server port
    printf("Enter server port (e.g., %d): ", DEFAULT_PORT);
    if (scanf("%d", &server_port) != 1) {
        fprintf(stderr, "Invalid port number entered. Using default %d.\n", DEFAULT_PORT);
        server_port = DEFAULT_PORT;
    }
    // Consume the newline character left in the input buffer by scanf
    int c;
    while ((c = getchar()) != '\n' && c != EOF);


    // 3. Initialize Winsock
    printf("Initialising Winsock...\n");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }
    printf("Initialised.\n");

    // 4. Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        fprintf(stderr, "Could not create socket: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    printf("Socket created.\n");

    // 5. Prepare the sockaddr_in structure
    server.sin_addr.s_addr = inet_addr(server_ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(server_port);

    // Check if inet_addr failed (returned INADDR_NONE)
    if (server.sin_addr.s_addr == INADDR_NONE) {
         fprintf(stderr, "Invalid IP address format: %s\n", server_ip);
         closesocket(sock);
         WSACleanup();
         return 1;
    }

    // 6. Connect to server
    printf("Connecting to %s:%d...\n", server_ip, server_port);
    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        fprintf(stderr, "Connect failed. Error Code: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    printf("Connected to server.\n");

    // 7. Communication loop
    while (1) {
        printf("Enter message ('exit' to quit): ");
        if (fgets(message, sizeof(message), stdin) == NULL) {
            // Handle potential read error or EOF
            printf("Input error or EOF detected. Exiting.\n");
            break;
        }

        // Remove trailing newline character
        message[strcspn(message, "\n")] = 0;

        // Check for exit command
        if (strcmp(message, "exit") == 0) {
            printf("Disconnecting...\n");
            break;
        }

        // Send the message
        if (send(sock, message, strlen(message), 0) < 0) {
            fprintf(stderr, "Send failed. Error Code: %d\n", WSAGetLastError());
            break; // Exit loop on send error
        }

        // Receive a reply from the server
        memset(recv_buffer, 0, sizeof(recv_buffer)); // Clear buffer before receiving
        int recv_len = recv(sock, recv_buffer, sizeof(recv_buffer) - 1, 0); // -1 for null terminator

        if (recv_len == SOCKET_ERROR) {
            fprintf(stderr, "recv failed. Error Code: %d\n", WSAGetLastError());
            break; // Exit loop on receive error
        } else if (recv_len == 0) {
            printf("Server disconnected gracefully.\n");
            break; // Server closed the connection
        } else {
            // Null-terminate the received data
            recv_buffer[recv_len] = '\0';
            printf("Server reply: %s\n", recv_buffer);
        }
    }

    // 8. Cleanup
    closesocket(sock);
    WSACleanup();
    printf("Connection closed.\n");
    return 0;
}