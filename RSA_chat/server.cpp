// g++ server.cpp -o server.exe -lws2_32; .\server.exe
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h> // Required for inet_ntop and INET_ADDRSTRLEN
#include <ctype.h>
#include <string.h> // Needed for strcmp and memset

#pragma comment(lib, "ws2_32.lib")  // Link with ws2_32.lib

#define BUFFER_SIZE 1024

// Capitalize function remains the same
char* capitalize(char* text) {
    if (text == NULL) return NULL;
    char* p = text;
    while (*p) {
        *p = toupper((unsigned char)*p); // Cast to unsigned char for safety
        p++;
    }
    return text;
}

int main() {
    WSADATA wsa;
    SOCKET server_socket = INVALID_SOCKET;
    SOCKET client_socket = INVALID_SOCKET;
    struct sockaddr_in server, client;
    int client_addr_len;
    int port;
    char client_msg[BUFFER_SIZE];

    // 1. Prompt for port number
    printf("Enter port number to host on: ");
    if (scanf("%d", &port) != 1) {
        fprintf(stderr, "Invalid port number entered.\n");
        return 1;
    }
    // Consume potential leftover newline
    int c;
    while ((c = getchar()) != '\n' && c != EOF);

    // 2. Initialize Winsock
    printf("Initialising Winsock...\n");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }
    printf("Initialised.\n");

    // 3. Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        fprintf(stderr, "Could not create socket: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    printf("Listening socket created.\n");

    // 4. Prepare sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY; // Listen on all available interfaces
    server.sin_port = htons(port);

    // 5. Bind
    if (bind(server_socket, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        fprintf(stderr, "Bind failed. Error Code: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }
    printf("Bind done.\n");

    // 6. Listen
    if (listen(server_socket, 5) == SOCKET_ERROR) { // Increased backlog slightly
        fprintf(stderr, "Listen failed. Error Code: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    printf("Server is listening on port %d...\n", port);

    // 7. Main Accept Loop (handles multiple client connections sequentially)
    client_addr_len = sizeof(struct sockaddr_in);
    while (1) { // Keep accepting new connections
        printf("\nWaiting for incoming connections...\n");
        client_socket = accept(server_socket, (struct sockaddr*)&client, &client_addr_len);

        if (client_socket == INVALID_SOCKET) {
            fprintf(stderr, "accept failed. Error Code: %d\n", WSAGetLastError());
            // Decide if the error is fatal or if we can continue listening
            // For simplicity here, we'll try to continue
            continue;
        }

        // Get client IP address and port for logging (optional but helpful)
        char client_ip[INET_ADDRSTRLEN];
        // inet_ntop(AF_INET, &client.sin_addr, client_ip, INET_ADDRSTRLEN);
        int client_port = ntohs(client.sin_port);
        printf("Connection accepted from %s:%d\n", client_ip, client_port);


        // --- Inner loop to handle messages from THIS client ---
        int recv_len;
        while( (recv_len = recv(client_socket, client_msg, sizeof(client_msg) - 1, 0)) > 0 )
        {
            // Null-terminate the received data
            client_msg[recv_len] = '\0';
            printf("Received from %s:%d: %s\n", client_ip, client_port, client_msg);

            // Check if client wants to exit
            if (strcmp(client_msg, "exit") == 0) {
                printf("Client %s:%d requested disconnect.\n", client_ip, client_port);
                // Optional: Send a confirmation/goodbye message
                // send(client_socket, "Goodbye!", strlen("Goodbye!"), 0);
                break; // Exit the inner message loop for this client
            }

            // Process the message (capitalize)
            capitalize(client_msg);

            // Send the response back to the client
            if (send(client_socket, client_msg, strlen(client_msg), 0) == SOCKET_ERROR) {
                 fprintf(stderr, "Send failed to %s:%d. Error Code: %d\n", client_ip, client_port, WSAGetLastError());
                 break; // Exit inner loop on send error
            }
             printf("Replied to %s:%d: %s\n", client_ip, client_port, client_msg);

             // Clear the buffer for the next message (good practice)
             memset(client_msg, 0, sizeof(client_msg));
        }
        // --- End of inner message loop ---


        // Check why the inner loop exited
        if (recv_len == 0) {
            printf("Client %s:%d disconnected gracefully.\n", client_ip, client_port);
        } else if (recv_len == SOCKET_ERROR) {
            // Don't print error again if it was a send failure handled above
            if (strcmp(client_msg, "exit") != 0) { // Avoid duplicate message if send failed after non-exit
                 fprintf(stderr, "recv failed for client %s:%d. Error Code: %d\n", client_ip, client_port, WSAGetLastError());
            }
        }

        // Close the socket FOR THIS CLIENT now that communication is done/failed
        closesocket(client_socket);
        client_socket = INVALID_SOCKET; // Reset for safety
        printf("Socket closed for client %s:%d\n", client_ip, client_port);

    } // End of main accept loop (while(1))

    // Cleanup (though this part might not be reached in a simple Ctrl+C exit)
    printf("Shutting down server...\n");
    closesocket(server_socket);
    WSACleanup();
    return 0;
}