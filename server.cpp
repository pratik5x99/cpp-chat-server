#include <iostream>      // For input/output operations (cout, cerr)
#include <sys/socket.h>  // For socket programming functions (socket, bind, listen, accept)
#include <netinet/in.h>  // For internet address structures (sockaddr_in)
#include <unistd.h>      // For read, write, and close functions
#include <string.h>      // For memset function

// Define a port number for the server to listen on
#define PORT 8080

int main() {
    // --- 1. Create a Socket ---
    // socket(domain, type, protocol)
    // AF_INET: Address Family for IPv4
    // SOCK_STREAM: Type for TCP (reliable, connection-oriented)
    // 0: Protocol, 0 means let the OS choose the appropriate protocol (which is TCP for SOCK_STREAM)
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Error: Socket creation failed." << std::endl;
        return -1;
    }

    // --- 2. Bind the Socket to an IP Address and Port ---
    sockaddr_in address;
    address.sin_family = AF_INET;         // Set the address family to IPv4
    address.sin_addr.s_addr = INADDR_ANY; // Bind to any available local IP address
    address.sin_port = htons(PORT);       // Convert port number to network byte order

    // bind(socket_fd, address_struct, address_length)
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::cerr << "Error: Bind failed." << std::endl;
        close(server_fd);
        return -1;
    }

    // --- 3. Listen for Incoming Connections ---
    // listen(socket_fd, backlog_queue_size)
    // 3 is a common backlog size for simple servers
    if (listen(server_fd, 3) < 0) {
        std::cerr << "Error: Listen failed." << std::endl;
        close(server_fd);
        return -1;
    }

    std::cout << "Server is listening on port " << PORT << "..." << std::endl;

    // --- 4. Accept an Incoming Connection ---
    int addrlen = sizeof(address);
    // accept() is a blocking call. It will wait here until a client connects.
    // It returns a new file descriptor for the connection with the client.
    int client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
    if (client_socket < 0) {
        std::cerr << "Error: Accept failed." << std::endl;
        close(server_fd);
        return -1;
    }

    std::cout << "Client connected!" << std::endl;

    // --- 5. Read and Write (The Conversation) ---
    char buffer[1024] = {0}; // A buffer to store data received from the client

    // Read data from the client
    int bytes_read = read(client_socket, buffer, 1024);
    if (bytes_read < 0) {
        std::cerr << "Error: Failed to read from socket." << std::endl;
    } else {
        std::cout << "Client says: " << buffer << std::endl;

        // Echo the message back to the client
        write(client_socket, buffer, strlen(buffer));
        std::cout << "Echoed message back to client." << std::endl;
    }

    // --- 6. Close the Sockets ---
    close(client_socket); // Close the connection with the current client
    close(server_fd);     // Close the listening socket
    
    return 0;
}
