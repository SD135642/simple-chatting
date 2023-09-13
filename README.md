#simple-chatting

#Chat Application

This project consists of two parts: chat_client.c and chat_server.c. These programs enable communication between a client 
and a server over a network using TCP/IP.

## chat_client.c
### Overview

The chat_client.c program allows a user to connect to a chat server, send messages, and receive responses. It includes 
features like username input, error handling, message length control, and clean socket closure.

### Usage

This program is designed to work in a Linux environment. To build, run the commands:
cd <repository's directory> 
make

To run the program, use the following command:

./chat_client <server_IP> <port>

    <server_IP>: The IP address of the target server.
    <port>: The port number on which the server is listening.

### Features

    Username Input: Allows users to choose a username with character limit validation.
    Error Handling: Provides detailed error messages and gracefully handles failures.
    Message Length Control: Enforces a maximum message length for efficient communication.
    Clean Socket Closure: Ensures proper closure of client sockets.

#chat_server.c
##Overview

The chat_server.c program establishes a server to which multiple clients can connect. It handles incoming connections, manages user sessions, and facilitates message broadcasting among connected clients.

### Usage
This program is designed to work in a Linux environment. To build, run the commands:
cd <repository's directory> 
make

To run the server, use the following command:

./chat_server <port_number>

    <port_number>: The port on which the server will listen for incoming connections (range: 1024-65535).

### Features

    Multiple Client Support: Allows multiple clients to connect concurrently.
    User Management: Assigns unique usernames to connected clients.
    Broadcasting: Sends messages from one client to all connected clients.
    Graceful Shutdown: Closes connections and releases resources properly.

### How to Use

    Compile the programs using a C compiler.
    Start the server by running ./chat_server <port> on the server machine.
    Clients can connect using ./chat_client <server_IP> <port> on their respective machines.

### Dependencies

Both programs rely on standard C libraries and a custom utility file (util.h) provided in this project.

### Notes

    Ensure that the server is active and listening on the specified IP and port.
    Customize the programs to suit specific requirements or add additional functionalities as needed.

### Author

Aleksandra Dubrovina

Contact

For inquiries or feedback, please contact aleks.dubrovina@gmail.com.

