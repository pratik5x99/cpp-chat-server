C++ Multi-threaded Chat Server
A high-performance, multi-threaded TCP chat server built from scratch in C++. This project demonstrates a deep understanding of low-level networking, concurrency, and resource management in a cross-platform environment.

Features
Multi-threaded Architecture: Handles multiple simultaneous client connections, with each client managed on a separate thread.

Real-time Broadcasting: Messages sent by one client are instantly broadcast to all other clients in the chat room.

Private Messaging: Users can send private messages to each other using the /msg <username> <message> command.

Dynamic & Unique Usernames:

Clients choose their own username upon connecting.

The server validates that all usernames are unique and re-prompts if a name is taken.

User-Friendly Interface:

Sends a welcome message with usage instructions to new clients.

Announces when users join or leave the chat.

Includes a graceful /quit command.

Cross-Platform Compatibility:

The server runs on macOS/Linux.

Successfully handles different client behaviors, including line-buffered (nc) and character-buffered (telnet) clients.

Correctly processes backspace characters from both Windows and Unix-like systems.

Robust & Stable:

Uses a std::mutex to protect shared data (client lists, username maps) from race conditions.

Includes the SO_REUSEADDR socket option to prevent "Bind failed" errors on quick restarts.

Technologies Used
Language: C++

Core Libraries: <sys/socket>, <thread>, <mutex>, <vector>, <map>, <sstream>

Concurrency: POSIX Threads (pthreads) via std::thread

Networking: POSIX Sockets API

How to Compile and Run
Prerequisites

A C++ compiler (like g++ or Clang)

A Unix-like operating system (macOS or Linux)

Compilation

The -pthread flag is required to link the multi-threading library.

g++ server.cpp -o server -pthread

Running the Server

./server

The server will start and listen on port 8080.

Connecting a Client

You can connect from another terminal using netcat or telnet.

# On macOS or Linux
nc 127.0.0.1 8080

# On Windows
telnet 127.0.0.1 8080

