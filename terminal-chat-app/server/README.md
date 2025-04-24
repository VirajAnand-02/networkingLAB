# Terminal Chat Application - Server Documentation

## Overview
This document provides instructions for compiling and running the TCP server component of the Terminal Chat Application. The server listens for incoming client connections, processes messages, and sends responses back to the clients.

## Functionality
- The server initializes Winsock and creates a TCP socket.
- It binds the socket to a user-specified port.
- The server listens for incoming client connections.
- Upon accepting a client, it receives messages, capitalizes them, and sends them back.
- The server continues to run until all clients have disconnected.

## Compilation Instructions
To compile the server, use the following command in your terminal:

```
gcc TCP_server_WIN.c -o server.exe -lws2_32
```

## Running the Server
1. Execute the compiled server program:

```
.\server.exe
```

2. When prompted, enter the port number on which the server should listen for incoming connections.

## Usage
- The server will display a message indicating that it is listening for connections.
- Once a client connects, the server will receive messages and respond with the capitalized version of the received message.
- The server will continue to accept new clients until all clients have disconnected.

## Notes
- Ensure that the specified port is open and not blocked by a firewall.
- The server can handle multiple clients, but it will close when all clients have disconnected.