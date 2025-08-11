#include <iostream>      // For input/output operations
#include <sys/socket.h>  // For socket programming functions
#include <netinet/in.h>  // For internet address structures
#include <unistd.h>      // For read, write, and close functions
#include <string.h>      // For memset function
#include <thread>        // For using std::thread for multi-threading
#include <vector>        // To store client sockets
#include <mutex>         // To protect shared data
#include <algorithm>     // For std::find
#include <map>           // To map client sockets to usernames

#define PORT 8080

// --- Shared Resources ---
// We need to protect both the list of clients and the map of usernames.
// One mutex is sufficient for both.
std::mutex shared_resources_mutex; 
std::vector<int> clients; 
std::map<int, std::string> client_usernames;

/**
 * @brief Broadcasts a message to all connected clients.
 * @param message The message string to be sent.
 * @param sender_socket The socket of the client who sent the message (optional, 0 to send to all).
 */
void broadcast_message(const std::string& message, int sender_socket = 0) {
    std::lock_guard<std::mutex> guard(shared_resources_mutex);

    for (int client_socket : clients) {
        // Send the message to every client that is NOT the sender
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
    char buffer[1024];
    std::string username;

    // --- 1. Get Username ---
    // Prompt client for their username
    write(client_socket, "Please enter your username: ", 28);
    memset(buffer, 0, 1024);
    int bytes_read = read(client_socket, buffer, 1024);
    if (bytes_read <= 0) {
        std::cout << "Client failed to provide username. Disconnecting." << std::endl;
        close(client_socket);
        return;
    }
    // Remove newline character from username if present
    username = std::string(buffer, bytes_read);
    username.erase(std::remove(username.begin(), username.end(), '\n'), username.end());
    username.erase(std::remove(username.begin(), username.end(), '\r'), username.end());


    // --- 2. Add client to shared resources and announce entry ---
    {
        std::lock_guard<std::mutex> guard(shared_resources_mutex);
        clients.push_back(client_socket);
        client_usernames[client_socket] = username;
    }
    
    std::string join_msg = "[Server]: " + username + " has joined the chat.\n";
    std::cout << join_msg;
    broadcast_message(join_msg, client_socket); // Announce to others

    // --- 3. Main Chat Loop ---
    while (true) {
        memset(buffer, 0, 1024);
        bytes_read = read(client_socket, buffer, 1024);

        if (bytes_read <= 0) {
            // Client has disconnected
            break; 
        }

        // Format the message with the username
        std::string message = "[" + username + "]: " + std::string(buffer);
        std::cout << message; // Log message to server console
        
        // Broadcast the message to all other clients
        broadcast_message(message, client_socket);
    }

    // --- 4. Cleanup for disconnected client ---
    // Announce departure
    std::string leave_msg = "[Server]: " + username + " has left the chat.\n";
    std::cout << leave_msg;
    broadcast_message(leave_msg, client_socket);

    // Remove client from shared resources
    {
        std::lock_guard<std::mutex> guard(shared_resources_mutex);
        client_usernames.erase(client_socket);
        auto it = std::find(clients.begin(), clients.end(), client_socket);
        if (it != clients.end()) {
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

        std::cout << "New client connection accepted." << std::endl;
        std::thread client_thread(handle_client, client_socket);
        client_thread.detach(); 
    }

    close(server_fd);
    return 0;
}
