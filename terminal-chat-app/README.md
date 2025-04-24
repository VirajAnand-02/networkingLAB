# Terminal Chat Application

This project implements a simple terminal-based chat application using TCP sockets. It consists of a server and a client, allowing users to communicate over a network.

## Project Structure

```
terminal-chat-app
├── server
│   ├── TCP_server_WIN.c   # TCP server implementation
│   └── README.md          # Documentation for the server
├── client
│   ├── TCP_client_WIN.c   # TCP client implementation
│   └── README.md          # Documentation for the client
└── README.md              # Main documentation for the project
```

## Features

- **Server**: 
  - Initializes Winsock and creates a socket.
  - Binds to a user-specified port and listens for incoming connections.
  - Accepts client connections and processes messages.
  - Capitalizes received messages and sends them back to the client.
  - Continues running until all clients disconnect.

- **Client**: 
  - Initializes Winsock and creates a socket.
  - Connects to the server using user-provided IP address and port number.
  - Prompts the user for messages and sends them to the server.
  - Receives responses from the server and displays them.
  - Disconnects when the user types "exit".

## Setup Instructions

1. **Compile the Server**:
   - Navigate to the `server` directory.
   - Use the command: `gcc TCP_server_WIN.c -o server.exe -lws2_32`
   - Run the server: `.\server.exe`

2. **Compile the Client**:
   - Navigate to the `client` directory.
   - Use the command: `gcc TCP_client_WIN.c -o client.exe -lws2_32`
   - Run the client: `.\client.exe`

## Usage

- Start the server first and provide a port number when prompted.
- Start the client and provide the server's IP address, port number, and a user identifier string.
- Users can send messages to each other until one of them types "exit", which will disconnect the client.
- The server will continue to run until all clients have disconnected.

## License

This project is open-source and available for modification and distribution.