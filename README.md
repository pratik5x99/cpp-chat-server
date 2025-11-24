🚀 C++ Multi-threaded Chat Server

A high-performance, multi-threaded TCP chat server built from scratch in C++. This project demonstrates a deep understanding of low-level networking, concurrency, and robust, cross-platform application design.

🔴 Live Demo: chat-room.ddns.net (Port 8080)

✨ Core Features

Multi-threaded Architecture: Handles multiple simultaneous client connections, with each client managed on a separate, detached thread for maximum responsiveness.

Chat Rooms: Users can create and join separate chat rooms using /join <room_name>.

Real-time Broadcasting: Messages sent by one client are instantly broadcast to all other clients in the same room.

Private Messaging: Users can send private messages to each other using the /msg <username> <message> command.

Dynamic & Unique Usernames:

Clients choose their own username upon connecting.

The server validates that all usernames are unique and re-prompts if a name is taken.

Prevents users from choosing reserved names like "Server" or "Admin".

User-Friendly Command Interface:

/join <room>: Join or create a specific chat room.

/list: Displays a list of all users in your current room.

/help: Shows the welcome message and command list.

/quit: Gracefully disconnects from the server.

Cross-Platform Compatibility:

A custom line-buffered reading protocol ensures seamless communication between netcat (macOS/Linux) and telnet (Windows) clients.

Correctly processes backspace characters from both Windows and Unix-like systems.

Robust & Stable:

Uses a std::mutex to protect shared data (rooms, client lists) from race conditions.

Includes the SO_REUSEADDR socket option to prevent "Bind failed" errors on quick restarts.

🛠️ Technologies Used

Language: C++

Core Libraries: <sys/socket>, <thread>, <mutex>, <vector>, <map>, <sstream>, <chrono>

Concurrency Model: Thread-per-client using POSIX Threads (pthreads) via std::thread

Networking Protocol: TCP/IP using the POSIX Sockets API

⚙️ How to Use the Chat Server

Option 1: Connect to the Live Demo (Easiest)

You can connect directly to the live instance running on AWS without installing anything.

On macOS or Linux:

nc chat-room.ddns.net 8080


On Windows (Command Prompt):

telnet chat-room.ddns.net 8080


Option 2: Run it Locally

If you want to run the server on your own machine:

Clone the repository:

git clone [https://github.com/pratik5x99/cpp-chat-server.git](https://github.com/pratik5x99/cpp-chat-server.git)


Compile:

g++ server.cpp -o server -pthread


Run:

./server


Connect: Open a new terminal and run nc 127.0.0.1 8080.