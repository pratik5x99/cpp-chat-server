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

// --- Shared Resources ---
std::mutex shared_resources_mutex; 
std::vector<int> clients; 
std::map<int, std::string> client_usernames;

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
 * @brief Broadcasts a message to all connected clients.
 * @param message The message string to be sent.
 * @param sender_socket The socket of the client who sent the message (optional, 0 to send to all).
 */
void broadcast_message(const std::string& message, int sender_socket = 0) {
    std::lock_guard<std::mutex> guard(shared_resources_mutex);

    for (int client_socket : clients) {
        if (client_socket != sender_socket) {
            write(client_socket, message.c_str(), message.length());
        }
    }
}

/**
 * @brief Sends a private message from one user to another.
 * @param message The message content.
 * @param sender_username The username of the person sending the message.
 * @param recipient_username The username of the intended recipient.
 * @param sender_socket The socket of the sender, to send confirmations/errors back.
 */
void send_private_message(const std::string& message, const std::string& sender_username, const std::string& recipient_username, int sender_socket) {
    std::lock_guard<std::mutex> guard(shared_resources_mutex);

    int recipient_socket = -1;
    // Find the recipient's socket by iterating through the map.
    for (auto const& pair : client_usernames) {
        if (pair.second == recipient_username) {
            recipient_socket = pair.first;
            break;
        }
    }

    if (recipient_socket != -1) {
        // Recipient was found, format and send the private message.
        std::string pm_to_send = get_current_time() + " [Private from " + sender_username + "]: " + message;
        write(recipient_socket, pm_to_send.c_str(), pm_to_send.length());

        // Send a confirmation message back to the sender.
        std::string confirmation_msg = get_current_time() + " [Server]: Your private message to " + recipient_username + " has been sent.\n";
        write(sender_socket, confirmation_msg.c_str(), confirmation_msg.length());
    } else {
        // Recipient was not found.
        std::string error_msg = get_current_time() + " [Server]: User '" + recipient_username + "' not found or is offline.\n";
        write(sender_socket, error_msg.c_str(), error_msg.length());
    }
}

/**
 * @brief Reads a full line (ending in '\n') from a socket.
 * This handles clients that send data character-by-character (like Telnet).
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
            break; // End of line
        }
        if (buffer == '\b' || buffer == '\x7f') { // Check for backspace (BS) or delete (DEL)
            if (!out_line.empty()) {
                out_line.pop_back();
            }
        } else if (buffer != '\r') { // Ignore carriage return
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
    std::string username;

    while (true) {
        write(client_socket, "Please enter your username: ", 28);
        if (!read_line_from_client(client_socket, username) || username.empty()) {
            std::cout << "Client failed to provide username. Disconnecting." << std::endl;
            close(client_socket);
            return;
        }

        bool name_taken = false;
        {
            std::lock_guard<std::mutex> guard(shared_resources_mutex);
            for (auto const& pair : client_usernames) {
                if (pair.second == username) {
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

    {
        std::lock_guard<std::mutex> guard(shared_resources_mutex);
        clients.push_back(client_socket);
        client_usernames[client_socket] = username;
    }
    
    std::string join_msg = get_current_time() + " [Server]: " + username + " has joined the chat.\n";
    std::cout << join_msg;
    broadcast_message(join_msg, client_socket);

    std::string help_msg = "\nWelcome to the C++ Chat Server, " + username + "!\n"
                           "========================================\n"
                           "Server Rules: Be respectful to others.\n\n"
                           "Commands:\n"
                           "  - To send a public message, just type and press Enter.\n"
                           "  - To send a private message: /msg <username> <message>\n"
                           "  - To see who is online: /list\n"
                           "  - To get help: /help\n"
                           "  - To leave the chat: /quit\n"
                           "========================================\n\n";
    write(client_socket, help_msg.c_str(), help_msg.length());

    std::string received_msg;
    while (read_line_from_client(client_socket, received_msg)) {
        if (received_msg == "/quit") {
            break;
        }
        else if (received_msg == "/list") {
            std::string user_list_msg = get_current_time() + " [Server]: Users currently online:\n";
            {
                std::lock_guard<std::mutex> guard(shared_resources_mutex);
                for (auto const& pair : client_usernames) {
                    user_list_msg += " - " + pair.second + "\n";
                }
            }
            write(client_socket, user_list_msg.c_str(), user_list_msg.length());
        }
        else if (received_msg == "/help") {
            write(client_socket, help_msg.c_str(), help_msg.length());
        }
        else if (received_msg.rfind("/msg", 0) == 0) {
            std::stringstream ss(received_msg);
            std::string command, recipient, private_message;
            
            ss >> command;
            ss >> recipient;
            
            std::getline(ss, private_message);
            size_t first_char = private_message.find_first_not_of(" \t");
            if (std::string::npos != first_char) {
                private_message = private_message.substr(first_char);
            }

            if (!recipient.empty() && !private_message.empty()) {
                send_private_message(private_message + "\n", username, recipient, client_socket);
            } else {
                std::string usage_msg = get_current_time() + " [Server]: Usage: /msg <username> <message>\n";
                write(client_socket, usage_msg.c_str(), usage_msg.length());
            }
        } else {
            std::string message = get_current_time() + " [" + username + "]: " + received_msg + "\n";
            std::cout << message;
            broadcast_message(message, client_socket);
        }
    }

    std::string leave_msg = get_current_time() + " [Server]: " + username + " has left the chat.\n";
    std::cout << leave_msg;
    broadcast_message(leave_msg, client_socket);

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
