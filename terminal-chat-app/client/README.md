# Terminal Chat Application Client

This README file provides instructions on how to compile and run the client for the Terminal Chat Application.

## Overview

The client connects to a TCP server, allowing users to send messages and receive responses. The client prompts the user for an IP address, port number, and a user identifier string. The communication continues until the user types "exit".

## Compilation

To compile the client, use the following command:

```
gcc TCP_client_WIN.c -o client.exe -lws2_32
```

## Running the Client

1. Ensure that the server is running and listening on the specified port.
2. Run the client executable:

```
.\client.exe
```

3. When prompted, enter the following:
   - **IP Address**: The IP address of the server (e.g., `127.0.0.1` for localhost).
   - **Port Number**: The port number on which the server is listening.
   - **User Identifier**: A string to identify the user in the chat.

4. Type your message and press Enter to send it to the server.
5. To exit the chat, type "exit" and press Enter.

## Functionality

- The client connects to the server using the provided IP address and port.
- It sends messages to the server and displays responses.
- The client will terminate when the user types "exit".

## Notes

- Ensure that the Winsock library is properly linked during compilation.
- The server must be running before starting the client for successful communication.