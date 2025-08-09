#include <iostream>      // For input/output operations (cout, cerr)
#include <sys/socket.h>  // For socket programming functions
#include <netinet/in.h>  // For internet address structures
#include <unistd.h>      // For read, write, and close functions
#include <string.h>      // For memset function
#include <thread>        // For using std::thread for multi-threading

#define PORT 8080

/**
 * @brief This function handles the communication with a single client.
 * It will be executed in a separate thread for each connected client.
 * @param client_socket The socket file descriptor for the connected client.
 */
void handle_client(int client_socket) {
    std::cout << "Client connected on thread: " << std::this_thread::get_id() << std::endl;

    char buffer[1024] = {0};

    // Loop to continuously read from the client
    while (true) {
        // Clear the buffer before reading
        memset(buffer, 0, 1024);

        // Read data from the client
        int bytes_read = read(client_socket, buffer, 1024);

        // If read() returns 0 or less, the client has disconnected
        if (bytes_read <= 0) {
            std::cout << "Client disconnected from thread: " << std::this_thread::get_id() << std::endl;
            close(client_socket);
            return; // Exit the function, which terminates the thread
        }

        std::cout << "Client says: " << buffer << std::endl;

        // Echo the message back to the client
        write(client_socket, buffer, strlen(buffer));
    }
}

int main() {
    int server_fd;
    sockaddr_in address;

    // --- 1. Create and configure the server socket ---
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

    if (listen(server_fd, 5) < 0) { // Increased backlog to 5
        std::cerr << "Error: Listen failed." << std::endl;
        close(server_fd);
        return -1;
    }

    std::cout << "Server is listening on port " << PORT << "..." << std::endl;

    // --- 2. The Main Server Loop ---
    // This loop will run forever, accepting new connections.
    while (true) {
        int client_socket = accept(server_fd, nullptr, nullptr);
        if (client_socket < 0) {
            std::cerr << "Error: Accept failed." << std::endl;
            continue; // Continue to the next iteration to wait for another client
        }

        // --- 3. Create a new thread to handle the client ---
        // std::thread(function_to_run, arguments_to_the_function...)
        // We pass the client_socket as an argument to our handle_client function.
        std::thread client_thread(handle_client, client_socket);
        
        // .detach() allows the thread to run independently in the background.
        // The main thread can immediately go back to accepting the next connection.
        client_thread.detach(); 
    }

    // The code below will not be reached in this simple server,
    // but it's good practice to have a cleanup mechanism.
    close(server_fd);
    
    return 0;
}
