#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <thread>

#define PORT 8080

void handle_client(int client_socket) {
    std::cout << "Client connected on thread: " << std::this_thread::get_id() << std::endl;

    char buffer[1024] = {0};

    while (true) {
        memset(buffer, 0, 1024);

        int bytes_read = read(client_socket, buffer, 1024);

        if (bytes_read <= 0) {
            std::cout << "Client disconnected from thread: " << std::this_thread::get_id() << std::endl;
            close(client_socket);
            return;
        }

        std::cout << "Client says: " << buffer << std::endl;

        write(client_socket, buffer, strlen(buffer));
    }
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

    std::cout << "Server is listening on port " << PORT << "..." << std::endl;

    while (true) {
        int client_socket = accept(server_fd, nullptr, nullptr);
        if (client_socket < 0) {
            std::cerr << "Error: Accept failed." << std::endl;
            continue;
        }

        std::thread client_thread(handle_client, client_socket);
        client_thread.detach();
    }

    close(server_fd);
    
    return 0;
}