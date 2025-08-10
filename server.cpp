#include <iostream>      // For input/output operations
#include <sys/socket.h>  // For socket programming functions
#include <netinet/in.h>  // For internet address structures
#include <unistd.h>      // For read, write, and close functions
#include <string.h>      // For memset function
#include <thread>        // For using std::thread for multi-threading
#include <vector>        // To store client sockets
#include <mutex>         // To protect shared data (the client list)
#include <algorithm>     // For std::find to remove elements from vector

#define PORT 8080

// --- Shared Resources ---
// Vector to store the socket file descriptors of all connected clients.
std::vector<int> clients; 
// Mutex to protect access to the shared 'clients' vector.
std::mutex clients_mutex; 

/**
 * @brief Broadcasts a message to all clients except the sender.
 * @param message The message string to be sent.
 * @param sender_socket The socket of the client who sent the message.
 */
void broadcast_message(const std::string& message, int sender_socket) {
    // Lock the mutex to ensure exclusive access to the clients vector.
    std::lock_guard<std::mutex> guard(clients_mutex);

    for (int client_socket : clients) {
        // Send the message to every client that is NOT the sender.
        if (client_socket != sender_socket) {
            write(client_socket, message.c_str(), message.length());
        }
    }
}

/**
 * @brief Handles all communication for a single client in a dedicated thread.
 * @param client_socket The socket file descriptor for the connected client.
 */
void handle_client(int client_socket) {
    // Add the new client to our list of clients.
    {
        std::lock_guard<std::mutex> guard(clients_mutex);
        clients.push_back(client_socket);
    }

    // Create a welcome message with the client's ID (their socket number)
    std::string welcome_msg = "Welcome! You are Client #" + std::to_string(client_socket) + "\n";
    write(client_socket, welcome_msg.c_str(), welcome_msg.length());

    char buffer[1024];

    // Loop to continuously read messages from this client
    while (true) {
        memset(buffer, 0, 1024);
        int bytes_read = read(client_socket, buffer, 1024);

        // If read() returns 0 or less, the client has disconnected.
        if (bytes_read <= 0) {
            std::cout << "Client #" << client_socket << " disconnected." << std::endl;
            break; // Exit the loop
        }

        // Format the message to be broadcasted
        std::string message = "[Client #" + std::to_string(client_socket) + "]: " + buffer;
        std::cout << message; // Log message to server console
        
        // Broadcast the message to all other clients
        broadcast_message(message, client_socket);
    }

    // --- Cleanup for disconnected client ---
    {
        std::lock_guard<std::mutex> guard(clients_mutex);
        // Find the disconnected client in the vector
        auto it = std::find(clients.begin(), clients.end(), client_socket);
        if (it != clients.end()) {
            // Remove it from the vector
            clients.erase(it);
        }
    }
    close(client_socket);
}

int main() {
    int server_fd;
    sockaddr_in address;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Error: Socket creation failed." << std::endl;
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::cerr << "Error: Bind failed." << std::endl;
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, 5) < 0) {
        std::cerr << "Error: Listen failed." << std::endl;
        close(server_fd);
        return -1;
    }

    std::cout << "Chat server is listening on port " << PORT << "..." << std::endl;

    while (true) {
        int client_socket = accept(server_fd, nullptr, nullptr);
        if (client_socket < 0) {
            std::cerr << "Error: Accept failed." << std::endl;
            continue;
        }

        std::cout << "New client connected: #" << client_socket << std::endl;
        std::thread client_thread(handle_client, client_socket);
        client_thread.detach(); 
    }

    close(server_fd);
    return 0;
}
