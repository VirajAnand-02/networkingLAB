// Compile: g++ server.cpp -o server.exe -lws2_32 -lstdc++
// Run:     .\server.exe
#define _WIN32_WINNT 0x0600

#include <process.h>
#include <stdio.h>
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

// Increase buffer size for encrypted messages
#define BUFFER_SIZE 8192  // Significantly larger

// --- Global Variables ---
volatile bool g_client_connected = false;
SOCKET g_client_socket =
    INVALID_SOCKET;  // Make socket accessible globally (use carefully!)

// Server's own keys
const std::vector<long long> server_public_key = {5, 323};     // {e, n}
const std::vector<long long> server_private_key = {173, 323};  // {d, n}

// To store the connected client's public key
std::vector<long long> client_public_key;

// Structure for thread arguments (now just needs a signal, socket is global)
typedef struct {
    HANDLE hEvent;  // Event to signal completion or error
} THREAD_ARGS;

// --- Receiver Thread Function ---
unsigned __stdcall ReceiveThreadFunc(void* args) {
    THREAD_ARGS* thread_args = (THREAD_ARGS*)args;
    char recv_buffer[BUFFER_SIZE];
    int recv_len;

    printf("[Receiver Thread] Started.\n");

    while (g_client_connected) {
        memset(recv_buffer, 0, BUFFER_SIZE);
        recv_len = recv(g_client_socket, recv_buffer, BUFFER_SIZE - 1, 0);

        if (recv_len > 0) {
            recv_buffer[recv_len] = '\0';  // Null-terminate C string
            std::string received_serialized(
                recv_buffer);  // Convert to C++ string

            // Display the encrypted string received
            printf("\n[Received Encrypted]: %s\n", received_serialized.c_str());

            // Deserialize and Decrypt
            std::vector<long long> ciphertext =
                deserialize_ciphertext(received_serialized);
            if (ciphertext.empty() && !received_serialized.empty()) {
                printf(
                    "\n[System] Received potentially invalid/empty "
                    "ciphertext.\n> ");
                continue;  // Skip processing if deserialization likely failed
            }
            std::string decrypted_message =
                decrypt(ciphertext, server_private_key);

            printf("[Decrypted Message]: %s\n> ",
                   decrypted_message.c_str());  // Print decrypted message
        } else if (recv_len == 0) {
            printf("\n[System] Client disconnected gracefully.\n> ");
            g_client_connected = false;
            break;
        } else {
            int error_code = WSAGetLastError();
            if (g_client_connected) {  // Avoid error msg if we initiated
                                       // disconnect
                if (error_code == WSAECONNRESET ||
                    error_code == WSAECONNABORTED) {
                    printf(
                        "\n[System] Client connection lost (reset/abort).\n> ");
                } else {
                    fprintf(stderr,
                            "\n[System] recv failed. Error Code: %d\n> ",
                            error_code);
                }
                g_client_connected = false;
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

// --- Main Server Logic ---
int main() {
    WSADATA wsa;
    SOCKET server_socket = INVALID_SOCKET;
    // client_socket is now global g_client_socket
    struct sockaddr_in server_addr, client_addr;
    int client_addr_len;
    int port;
    char comm_buffer[BUFFER_SIZE];  // General communication buffer

    // --- Setup --- (Same as before)
    printf("Enter port number to host on: ");
    if (scanf("%d", &port) != 1) {
        fprintf(stderr, "Invalid port.\n");
        return 1;
    }
    int c;
    while ((c = getchar()) != '\n' && c != EOF);

    printf("Initialising Winsock...\n");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) { /* ... error handling ... */
        return 1;
    }
    printf("Initialised.\n");

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) { /* ... error handling ... */
        WSACleanup();
        return 1;
    }
    printf("Listening socket created.\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr*)&server_addr,
             sizeof(server_addr)) ==
        SOCKET_ERROR) { /* ... error handling ... */
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }
    printf("Bind done.\n");

    if (listen(server_socket, 1) == SOCKET_ERROR) { /* ... error handling ... */
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }
    printf("Server is listening on port %d...\n", port);

    // --- Main Accept Loop ---
    while (1) {
        printf("\n[System] Waiting for incoming connection...\n");
        client_addr_len = sizeof(struct sockaddr_in);
        g_client_socket = accept(server_socket, (struct sockaddr*)&client_addr,
                                 &client_addr_len);

        if (g_client_socket == INVALID_SOCKET) { /* ... error handling ... */
            continue;
        }

        // Client connected! Reset state
        g_client_connected = true;
        client_public_key.clear();  // Clear previous client key

        char client_ip_str[INET_ADDRSTRLEN];
        // inet_ntop(AF_INET, &client_addr.sin_addr, client_ip_str,
        // INET_ADDRSTRLEN);
        int client_port_num = ntohs(client_addr.sin_port);
        printf("[System] Connection accepted from %s:%d\n", client_ip_str,
               client_port_num);

        // --- Key Exchange ---
        bool key_exchange_ok = false;
        try {
            // 1. Send Server Public Key {e, n}
            snprintf(comm_buffer, sizeof(comm_buffer), "%lld %lld",
                     server_public_key[0], server_public_key[1]);
            printf("[System] Sending server key: %s\n", comm_buffer);
            if (send(g_client_socket, comm_buffer, strlen(comm_buffer), 0) <=
                0) {
                throw std::runtime_error("Failed to send server key");
            }

            // 2. Receive Client Public Key {e, n}
            memset(comm_buffer, 0, sizeof(comm_buffer));
            int key_recv_len =
                recv(g_client_socket, comm_buffer, sizeof(comm_buffer) - 1, 0);
            if (key_recv_len <= 0) {
                throw std::runtime_error("Failed to receive client key");
            }
            comm_buffer[key_recv_len] = '\0';
            printf("[System] Received client key string: %s\n", comm_buffer);

            long long client_e, client_n;
            if (sscanf(comm_buffer, "%lld %lld", &client_e, &client_n) == 2) {
                client_public_key.push_back(client_e);
                client_public_key.push_back(client_n);
                printf("[System] Parsed client key: { e=%lld, n=%lld }\n",
                       client_public_key[0], client_public_key[1]);
                key_exchange_ok = true;
            } else {
                throw std::runtime_error("Failed to parse client key string");
            }
        } catch (const std::exception& e) {
            fprintf(stderr,
                    "[System] Key exchange failed: %s. Error Code: %d\n",
                    e.what(), WSAGetLastError());
            closesocket(g_client_socket);
            g_client_socket = INVALID_SOCKET;
            g_client_connected = false;
            continue;  // Go back to accept
        }

        if (!key_exchange_ok || client_public_key.empty()) {
            fprintf(stderr,
                    "[System] Key exchange protocol failed logically.\n");
            closesocket(g_client_socket);
            g_client_socket = INVALID_SOCKET;
            g_client_connected = false;
            continue;  // Go back to accept
        }
        printf("[System] Key exchange successful. Starting chat session.\n");

        // --- Start Receiver Thread ---
        HANDLE hRecvEvent = CreateEvent(
            NULL, TRUE, FALSE, NULL);  // Manual reset, initially non-signaled
        THREAD_ARGS* args = (THREAD_ARGS*)malloc(sizeof(THREAD_ARGS));
        if (!args || !hRecvEvent) { /* ... handle allocation errors ... */
            if (hRecvEvent) CloseHandle(hRecvEvent);
            if (args) free(args);
            closesocket(g_client_socket);
            g_client_connected = false;
            continue;
        }
        args->hEvent = hRecvEvent;

        HANDLE hRecvThread =
            (HANDLE)_beginthreadex(NULL, 0, &ReceiveThreadFunc, args, 0, NULL);
        if (hRecvThread == NULL) { /* ... handle thread creation error ... */
            CloseHandle(hRecvEvent);
            free(args);
            closesocket(g_client_socket);
            g_client_connected = false;
            continue;
        }

        // --- Main Thread Handles Sending ---
        printf("> ");
        std::string line_buffer;  // Use C++ string for input reading
                                  // flexibility if needed
        while (g_client_connected) {
            // Using fgets for simplicity with C buffer
            if (fgets(comm_buffer, sizeof(comm_buffer), stdin) != NULL) {
                comm_buffer[strcspn(comm_buffer, "\n")] = 0;  // Remove newline
                std::string plaintext_message(
                    comm_buffer);  // Convert to C++ string

                if (!g_client_connected) break;

                if (plaintext_message == "exit") {
                    printf("[System] Server initiated disconnect.\n");
                    g_client_connected = false;  // Signal receiver
                    // Don't encrypt the "exit" command itself, just break
                    break;
                }

                // Encrypt and Serialize
                std::vector<long long> ciphertext =
                    encrypt(plaintext_message, client_public_key);
                std::string serialized_ciphertext =
                    serialize_ciphertext(ciphertext);

                // Send encrypted message
                if (send(g_client_socket, serialized_ciphertext.c_str(),
                         serialized_ciphertext.length(), 0) <= 0) {
                    if (!g_client_connected) { /* Client disconnected
                                                  concurrently */
                    } else {                   /* Handle send error */
                        fprintf(stderr,
                                "[System] send failed. Error Code: %d\n",
                                WSAGetLastError());
                        g_client_connected = false;
                    }
                    break;
                }
                printf("> ");  // Re-prompt
            } else {
                // fgets failed
                printf("\n[System] Console input error/EOF. Disconnecting.\n");
                g_client_connected = false;
                break;
            }
        }  // End sending loop

        // --- Cleanup for this Client ---
        g_client_connected = false;  // Ensure flag is false
        printf("[System] Disconnecting client...\n");

        // Optionally send an unencrypted disconnect notification? Risky if
        // receiver expects encrypted. Better: rely on socket closure.

        shutdown(g_client_socket, SD_BOTH);  // Shutdown socket
        closesocket(g_client_socket);        // Close socket handle
        g_client_socket = INVALID_SOCKET;    // Reset global socket variable

        printf("[System] Waiting for receiver thread to finish...\n");
        // Wait for receiver thread to signal completion or timeout
        if (hRecvThread) {
            WaitForSingleObject(hRecvEvent, 5000);  // Wait up to 5 seconds
            CloseHandle(
                hRecvThread);  // Close thread handle regardless of wait result
        }
        if (hRecvEvent) CloseHandle(hRecvEvent);  // Close event handle

        printf("[System] Client disconnected. Ready for next connection.\n");

    }  // End accept loop

    // --- Final Server Cleanup --- (Likely not reached)
    printf("Shutting down server...\n");
    if (server_socket != INVALID_SOCKET) closesocket(server_socket);
    WSACleanup();
    return 0;
}