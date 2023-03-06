# Computer-Networks
The given code is a server implementation in C language using sockets. It listens for incoming client connections, receives messages from clients, and stores user information in a file. It also handles user disconnections and provides a list of currently connected users to clients. It seems like a multi-client chat server that allows clients to communicate with each other.

The server uses POSIX threads to handle multiple clients concurrently. It creates a new thread for each client connection that is accepted by the server. Each thread reads incoming messages from its client socket and processes them based on the command in the message. The server uses semaphores to ensure mutual exclusion when writing to a file.

The server provides the following functionality:

When a client connects, the server sends a message "HELO" to the client, and expects the client to respond with its name using the "MESG" command.
The server appends the user name and IP address to a file named "user.txt".
The server can provide a list of currently connected users to a client using the "LIST" command.
The server can handle private messages between clients using the "MESG" command. The client specifies the recipient of the message using the recipient's name and IP address in the format "name@ip".
