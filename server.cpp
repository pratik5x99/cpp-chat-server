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
#include <sstream>       // For parsing strings
#include <chrono>        // For getting the current time
#include <iomanip>       // For formatting the time

#define PORT 8080
#define DEFAULT_ROOM "#general"

// --- Struct to hold user information ---
struct UserInfo {
    std::string username;
    std::string current_room;
};

// --- Shared Resources ---
std::mutex shared_resources_mutex; 
std::map<int, UserInfo> clients_info; // Maps socket to user info
std::map<std::string, std::vector<int>> rooms; // Maps room name to list of client sockets

/**
 * @brief Gets the current time formatted as [HH:MM].
 * @return A string with the formatted current time.
 */
std::string get_current_time() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "[%H:%M]");
    return ss.str();
}

/**
 * @brief Broadcasts a message to all clients in a specific room.
 * @param message The message string to be sent.
 * @param room The name of the room to broadcast to.
 * @param sender_socket The socket of the client who sent the message.
 */
void broadcast_message(const std::string& message, const std::string& room, int sender_socket) {
    std::lock_guard<std::mutex> guard(shared_resources_mutex);

    // Check if the room exists and has members
    if (rooms.find(room) != rooms.end()) {
        for (int client_socket : rooms.at(room)) {
            if (client_socket != sender_socket) {
                write(client_socket, message.c_str(), message.length());
            }
        }
    }
}

/**
 * @brief Reads a full line (ending in '\n') from a socket.
 * @param client_socket The socket to read from.
 * @param out_line The string to store the resulting line.
 * @return bool True if a line was read successfully, false if the client disconnected.
 */
bool read_line_from_client(int client_socket, std::string& out_line) {
    out_line = "";
    char buffer;
    int bytes_read;

    while ((bytes_read = read(client_socket, &buffer, 1)) > 0) {
        if (buffer == '\n') {
            break;
        }
        if (buffer == '\b' || buffer == '\x7f') {
            if (!out_line.empty()) {
                out_line.pop_back();
            }
        } else if (buffer != '\r') {
            out_line += buffer;
        }
    }
    return bytes_read > 0;
}


/**
 * @brief Handles all communication for a single client in a dedicated thread.
 * @param client_socket The socket file descriptor for the connected client.
 */
void handle_client(int client_socket) {
    UserInfo user;
    user.current_room = DEFAULT_ROOM;

    // Get a unique username
    while (true) {
        write(client_socket, "Please enter your username: ", 28);
        if (!read_line_from_client(client_socket, user.username) || user.username.empty()) {
            std::cout << "Client failed to provide username. Disconnecting." << std::endl;
            close(client_socket);
            return;
        }

        bool name_taken = false;
        std::string lower_username = user.username;
        std::transform(lower_username.begin(), lower_username.end(), lower_username.begin(), ::tolower);

        if (lower_username == "server" || lower_username == "admin") {
             write(client_socket, "[Server]: That username is reserved. Please try another.\n", 56);
             continue;
        }

        {
            std::lock_guard<std::mutex> guard(shared_resources_mutex);
            for (auto const& pair : clients_info) {
                if (pair.second.username == user.username) {
                    name_taken = true;
                    break;
                }
            }
        }

        if (name_taken) {
            write(client_socket, "[Server]: Username is already taken. Please try another.\n", 55);
        } else {
            break;
        }
    }

    // Add client to shared resources and default room
    {
        std::lock_guard<std::mutex> guard(shared_resources_mutex);
        clients_info[client_socket] = user;
        rooms[user.current_room].push_back(client_socket);
    }
    
    std::string join_msg = get_current_time() + " [Server]: " + user.username + " has joined " + user.current_room + ".\n";
    std::cout << join_msg;
    broadcast_message(join_msg, user.current_room, client_socket);

    std::string help_msg = "\nWelcome to the C++ Chat Server, " + user.username + "!\n"
                           "========================================\n"
                           "You are currently in room: " + user.current_room + "\n\n"
                           "Commands:\n"
                           "  - /join <room_name>   - Join or create a new room.\n"
                           "  - /msg <username> <message> - Send a private message.\n"
                           "  - /list                 - See who is in your current room.\n"
                           "  - /help                 - Show this help message again.\n"
                           "  - /quit                 - Leave the chat.\n"
                           "========================================\n\n";
    write(client_socket, help_msg.c_str(), help_msg.length());

    std::string received_msg;
    while (read_line_from_client(client_socket, received_msg)) {
        if (received_msg == "/quit") {
            break;
        }
        else if (received_msg == "/list") {
            std::string user_list_msg = get_current_time() + " [Server]: Users in " + user.current_room + ":\n";
            {
                std::lock_guard<std::mutex> guard(shared_resources_mutex);
                for (int member_socket : rooms[user.current_room]) {
                    user_list_msg += " - " + clients_info[member_socket].username + "\n";
                }
            }
            write(client_socket, user_list_msg.c_str(), user_list_msg.length());
        }
        else if (received_msg == "/help") {
            write(client_socket, help_msg.c_str(), help_msg.length());
        }
        // --- NEW: Handle /join command ---
        else if (received_msg.rfind("/join", 0) == 0) {
            std::stringstream ss(received_msg);
            std::string command, new_room;
            ss >> command >> new_room;

            if (!new_room.empty() && new_room != user.current_room) {
                // Announce departure from old room
                std::string leave_room_msg = get_current_time() + " [Server]: " + user.username + " has left the room.\n";
                broadcast_message(leave_room_msg, user.current_room, client_socket);

                // Update server state
                {
                    std::lock_guard<std::mutex> guard(shared_resources_mutex);
                    // Remove from old room's vector
                    auto& old_room_clients = rooms[user.current_room];
                    old_room_clients.erase(std::remove(old_room_clients.begin(), old_room_clients.end(), client_socket), old_room_clients.end());
                    
                    // Update user's current room
                    user.current_room = new_room;
                    clients_info[client_socket].current_room = new_room;
                    
                    // Add to new room's vector
                    rooms[new_room].push_back(client_socket);
                }

                // Announce arrival in new room
                std::string join_room_msg = get_current_time() + " [Server]: " + user.username + " has joined " + new_room + ".\n";
                broadcast_message(join_room_msg, new_room, 0); // Send to everyone in new room
                write(client_socket, ("You have joined " + new_room + ".\n").c_str(), ("You have joined " + new_room + ".\n").length());
            }
        }
        else if (received_msg.rfind("/msg", 0) == 0) {
            // Private messaging logic would need to be updated to search across all rooms
            // For simplicity, this is left as an exercise.
            write(client_socket, "[Server]: Private messaging is not yet implemented in rooms.\n", 60);
        } else {
            std::string message = get_current_time() + " [" + user.username + "]: " + received_msg + "\n";
            std::cout << user.current_room << " " << message;
            broadcast_message(message, user.current_room, client_socket);
        }
    }

    // Cleanup for disconnected client
    std::string leave_msg = get_current_time() + " [Server]: " + user.username + " has left the chat.\n";
    std::cout << leave_msg;
    broadcast_message(leave_msg, user.current_room, client_socket);

    {
        std::lock_guard<std::mutex> guard(shared_resources_mutex);
        
        // 1. Remove from the room they were in
        auto& room_clients = rooms[user.current_room];
        room_clients.erase(std::remove(room_clients.begin(), room_clients.end(), client_socket), room_clients.end());
        
        // If the room is now empty (and not the default), you might want to delete it to save memory:
        if (room_clients.empty() && user.current_room != DEFAULT_ROOM) {
            rooms.erase(user.current_room);
        }

        // 2. Remove from the global clients info map
        // THIS IS THE KEY FIX: Ensuring the username is freed up.
        clients_info.erase(client_socket);
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

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        std::cerr << "Error: setsockopt failed." << std::endl;
        close(server_fd);
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