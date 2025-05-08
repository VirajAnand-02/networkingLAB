// Compile: g++ client.cpp -o client.exe -lws2_32 -lstdc++
// Run:     .\client.exe
#define _WIN32_WINNT 0x0600

#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <iostream>  // For std::cerr, std::cout (optional)
#include <string>    // Include C++ string
#include <vector>    // Include C++ vector

// Include our RSA header
#include "rsa_chat.hpp"

#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT 8080
#define DEFAULT_IP "127.0.0.1"
// Increase buffer size for encrypted messages
#define BUFFER_SIZE 8192  // Significantly larger

// --- Global Variables ---
volatile bool g_connection_active = false;
SOCKET g_sock = INVALID_SOCKET;  // Make socket global (use carefully!)

// Client's own keys (DIFFERENT from server's!)
// Let's generate a different simple pair: p=13, q=23 => n=299, phi=264. e=7,
// d=151
const std::vector<long long> client_public_key = {7, 299};     // {e, n}
const std::vector<long long> client_private_key = {151, 299};  // {d, n}

// To store the connected server's public key
std::vector<long long> server_public_key;

// Structure for thread arguments
typedef struct {
    HANDLE hEvent;  // Event to signal completion or error
} RECEIVER_THREAD_ARGS;

// --- Receiver Thread Function ---
unsigned __stdcall ClientReceiveThreadFunc(void* args) {
    RECEIVER_THREAD_ARGS* thread_args = (RECEIVER_THREAD_ARGS*)args;
    char recv_buffer[BUFFER_SIZE];
    int recv_len;

    printf("[Receiver Thread] Started.\n");

    while (g_connection_active) {
        memset(recv_buffer, 0, BUFFER_SIZE);
        recv_len = recv(g_sock, recv_buffer, BUFFER_SIZE - 1, 0);

        if (recv_len > 0) {
            recv_buffer[recv_len] = '\0';
            std::string received_serialized(recv_buffer);

            // Display the encrypted string received
            printf("\n[Received Encrypted]: %s\n", received_serialized.c_str());

            // Deserialize and Decrypt
            std::vector<long long> ciphertext =
                deserialize_ciphertext(received_serialized);
            if (ciphertext.empty() && !received_serialized.empty()) {
                printf(
                    "\n[System] Received potentially invalid/empty "
                    "ciphertext.\n> ");
                continue;
            }
            std::string decrypted_message =
                decrypt(ciphertext, client_private_key);

            printf("[Decrypted Message]: %s\n> ",
                   decrypted_message.c_str());  // Print decrypted message
        } else if (recv_len == 0) {
            printf("\n[System] Server disconnected.\n> ");
            g_connection_active = false;
            break;
        } else {
            int error_code = WSAGetLastError();
            if (g_connection_active) {  // Avoid error msg if we initiated
                                        // disconnect
                if (error_code == WSAECONNRESET ||
                    error_code == WSAECONNABORTED) {
                    printf("\n[System] Connection to server lost.\n> ");
                } else {
                    fprintf(stderr,
                            "\n[System] recv failed. Error Code: %d\n> ",
                            error_code);
                }
                g_connection_active = false;
            }
            break;
        }
    }

    printf("[Receiver Thread] Exiting.\n");
    if (thread_args && thread_args->hEvent)
        SetEvent(thread_args->hEvent);  // Signal completion
    free(thread_args);                  // Free arguments
    _endthreadex(0);
    return 0;
}

// --- Main Client Logic ---
int main() {
    WSADATA wsa;
    // sock is now global g_sock
    struct sockaddr_in server_addr;
    char server_ip_str[16];
    int server_port_num;
    char comm_buffer[BUFFER_SIZE];  // General comm buffer

    // --- Setup --- (Get IP/Port - same as before)
    printf("Enter server IP address (leave blank for %s): ", DEFAULT_IP);
    if (fgets(server_ip_str, sizeof(server_ip_str), stdin) != NULL) {
        server_ip_str[strcspn(server_ip_str, "\n")] = 0;
        if (server_ip_str[0] == '\0') {
            strcpy(server_ip_str, DEFAULT_IP);
            printf("Using default IP: %s\n", server_ip_str);
        }
    } else { /* ... error handling ... */
        return 1;
    }

    printf("Enter server port (e.g., %d): ", DEFAULT_PORT);
    if (scanf("%d", &server_port_num) != 1) {
        server_port_num = DEFAULT_PORT;
        fprintf(stderr, "Invalid port. Using default %d.\n", server_port_num);
    }
    int c;
    while ((c = getchar()) != '\n' && c != EOF);  // Consume newline

    // --- Initialize Winsock, Create Socket --- (Same as before)
    printf("Initialising Winsock...\n");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) { /* ... error handling ... */
        return 1;
    }
    printf("Initialised.\n");

    if ((g_sock = socket(AF_INET, SOCK_STREAM, 0)) ==
        INVALID_SOCKET) { /* ... error handling ... */
        WSACleanup();
        return 1;
    }
    printf("Socket created.\n");

    // --- Prepare Server Address --- (Same as before)
    server_addr.sin_addr.s_addr = inet_addr(server_ip_str);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port_num);
    if (server_addr.sin_addr.s_addr ==
        INADDR_NONE) { /* ... error handling ... */
        closesocket(g_sock);
        WSACleanup();
        return 1;
    }

    // --- Connect to Server ---
    printf("Connecting to %s:%d...\n", server_ip_str, server_port_num);
    if (connect(g_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) <
        0) {
        fprintf(stderr, "Connect failed. Error Code: %d\n", WSAGetLastError());
        closesocket(g_sock);
        WSACleanup();
        return 1;
    }
    printf("[System] Connected to server.\n");
    g_connection_active = true;  // Set flag EARLY before key exchange
    server_public_key.clear();   // Clear any previous key

    // --- Key Exchange ---
    bool key_exchange_ok = false;
    try {
        // 1. Receive Server Public Key {e, n}
        memset(comm_buffer, 0, sizeof(comm_buffer));
        int key_recv_len =
            recv(g_sock, comm_buffer, sizeof(comm_buffer) - 1, 0);
        if (key_recv_len <= 0) {
            throw std::runtime_error("Failed to receive server key");
        }
        comm_buffer[key_recv_len] = '\0';
        printf("[System] Received server key string: %s\n", comm_buffer);

        long long server_e, server_n;
        if (sscanf(comm_buffer, "%lld %lld", &server_e, &server_n) == 2) {
            server_public_key.push_back(server_e);
            server_public_key.push_back(server_n);
            printf("[System] Parsed server key: { e=%lld, n=%lld }\n",
                   server_public_key[0], server_public_key[1]);
        } else {
            throw std::runtime_error("Failed to parse server key string");
        }

        // 2. Send Client Public Key {e, n}
        snprintf(comm_buffer, sizeof(comm_buffer), "%lld %lld",
                 client_public_key[0], client_public_key[1]);
        printf("[System] Sending client key: %s\n", comm_buffer);
        if (send(g_sock, comm_buffer, strlen(comm_buffer), 0) <= 0) {
            throw std::runtime_error("Failed to send client key");
        }

        key_exchange_ok = true;

    } catch (const std::exception& e) {
        fprintf(stderr, "[System] Key exchange failed: %s. Error Code: %d\n",
                e.what(), WSAGetLastError());
        closesocket(g_sock);
        g_sock = INVALID_SOCKET;
        g_connection_active = false;  // Mark inactive
        WSACleanup();
        return 1;  // Exit client if key exchange fails
    }

    if (!key_exchange_ok || server_public_key.empty()) {
        fprintf(stderr, "[System] Key exchange protocol failed logically.\n");
        closesocket(g_sock);
        g_sock = INVALID_SOCKET;
        g_connection_active = false;
        WSACleanup();
        return 1;
    }
    printf("[System] Key exchange successful. Starting chat session.\n");

    // --- Start Receiver Thread ---
    HANDLE hRecvEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    RECEIVER_THREAD_ARGS* args =
        (RECEIVER_THREAD_ARGS*)malloc(sizeof(RECEIVER_THREAD_ARGS));
    if (!args || !hRecvEvent) { /* ... handle allocation errors ... */
        if (hRecvEvent) CloseHandle(hRecvEvent);
        if (args) free(args);
        closesocket(g_sock);
        g_connection_active = false;
        WSACleanup();
        return 1;
    }
    args->hEvent = hRecvEvent;

    HANDLE hRecvThread = (HANDLE)_beginthreadex(
        NULL, 0, &ClientReceiveThreadFunc, args, 0, NULL);
    if (hRecvThread == NULL) { /* ... handle thread creation error ... */
        CloseHandle(hRecvEvent);
        free(args);
        closesocket(g_sock);
        g_connection_active = false;
        WSACleanup();
        return 1;
    }

    // --- Main Thread: Send Loop ---
    printf("> ");
    std::string line_buffer;
    while (g_connection_active) {
        if (fgets(comm_buffer, sizeof(comm_buffer), stdin) != NULL) {
            comm_buffer[strcspn(comm_buffer, "\n")] = 0;
            std::string plaintext_message(comm_buffer);

            if (!g_connection_active) break;

            // Encrypt and Serialize
            std::vector<long long> ciphertext =
                encrypt(plaintext_message, server_public_key);
            std::string serialized_ciphertext =
                serialize_ciphertext(ciphertext);

            // Send encrypted message
            if (send(g_sock, serialized_ciphertext.c_str(),
                     serialized_ciphertext.length(), 0) <= 0) {
                if (!g_connection_active) { /* Server disconnected concurrently
                                             */
                } else {                    /* Handle send error */
                    fprintf(stderr, "[System] send failed. Error Code: %d\n",
                            WSAGetLastError());
                    g_connection_active = false;
                }
                break;
            }

            // Check exit condition *after* sending encrypted "exit"
            if (plaintext_message == "exit") {
                printf("[System] Disconnecting...\n");
                g_connection_active = false;  // Signal receiver
                break;
            }
            printf("> ");  // Re-prompt
        } else {
            // fgets failed
            printf("\n[System] Console input error/EOF. Disconnecting...\n");
            g_connection_active = false;
            // Optionally send encrypted disconnect message?
            break;
        }
    }  // End sending loop

    // --- Cleanup ---
    g_connection_active = false;  // Ensure flag is false
    printf("[System] Shutting down sending side...\n");

    shutdown(g_sock, SD_SEND);  // Signal no more data from client

    printf("[System] Waiting for receiver thread to finish...\n");
    if (hRecvThread) {
        WaitForSingleObject(hRecvEvent, 5000);  // Wait up to 5 seconds
        CloseHandle(hRecvThread);
    }
    if (hRecvEvent) CloseHandle(hRecvEvent);

    printf("[System] Receiver thread finished.\n");

    closesocket(g_sock);      // Close socket
    g_sock = INVALID_SOCKET;  // Reset global
    WSACleanup();             // Cleanup Winsock
    printf("[System] Connection closed.\n");
    return 0;
}