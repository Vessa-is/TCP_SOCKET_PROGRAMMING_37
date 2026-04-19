# TCP Socket Programming - Multi-Client Server with HTTP Monitoring

## Overview

This project implements a **TCP-based multi-client chat-like server** in C++ using Windows Sockets (Winsock2). It supports up to 3 concurrent clients and includes a built-in lightweight HTTP server for real-time monitoring of server statistics.

### What it does
- A central TCP server listens on port **54000** and accepts connections from multiple clients.
- Clients can send arbitrary text messages; the server logs them with timestamps and client IP.
- The server recognizes special commands: `STATUS` and `BYE`.
- An embedded HTTP server runs on port **8080**, exposing a `/stats` endpoint that returns JSON with active clients, IPs, total messages, and full message history.

### Problem it solves
Demonstrates core concepts of TCP socket programming on Windows, including:
- Concurrent client handling with threads
- Connection limiting
- Message logging
- Timeout management
- Simple HTTP monitoring interface

### Key Features
- Support for up to 3 simultaneous clients (others are rejected)
- Per-client receive timeout (10 seconds)
- Persistent message logging to `msg_logs.txt`
- Thread-safe logging and state management
- HTTP/JSON stats endpoint at `http://localhost:8080/stats`
- Graceful handling of `BYE` command and client disconnections

## Architecture / Design

### High-level Design
- **Main TCP Server**: Binds to port 54000, accepts connections, and spawns a dedicated thread per client (up to `MAX_CLIENTS = 3`).
- **Client Handler**: Processes incoming messages, logs them, responds to special commands, and handles timeouts/disconnections.
- **HTTP Monitoring Server**: Runs in a detached background thread on port 8080. Responds only to `GET /stats` with current server state in JSON format.
- **Shared State**: Protected by mutexes — active client count, list of client IPs, message log vector, and total message counter.

### Data Flow
1. Client connects to server:54000 → server spawns `handleClient` thread.
2. Client sends message → server receives, logs to console + `msg_logs.txt`, increments counter.
3. Server replies based on message (`STATUS`, `BYE`, or unknown).
4. HTTP client requests `GET /stats` → server returns current JSON snapshot.

## Tech Stack

- **Language**: C++ (Windows-specific)
- **Networking**: Winsock2 (`winsock2.h`, `ws2tcpip.h`)
- **Libraries**: Standard C++ Library (`<thread>`, `<vector>`, `<mutex>`, `<atomic>`, `<fstream>`, etc.)
- **Platform**: Windows (uses `WSADATA`, `SOCKET`, `sockaddr_in`, `#pragma comment(lib, "ws2_32.lib")`)
- **No external dependencies** beyond the Windows SDK

**Note**: The codebase is Windows-only due to Winsock usage and `localtime_s`.

## Installation

### Prerequisites
- Windows OS
- Visual Studio (recommended) or any C++ compiler supporting Winsock2 (MinGW with Winsock support)
- Administrator privileges not required

### Build Steps
1. Clone the repository:
   ```bash
   git clone https://github.com/Vessa-is/TCP_SOCKET_PROGRAMMING_37.git
   cd TCP_SOCKET_PROGRAMMING_37

Open the project in Visual Studio:
Create a new Win32 Console Application project (or use existing .sln if present).
Add all .cpp files (server.cpp, client1.cpp, client2.cpp, client3.cpp).

Alternatively, compile from command line (using Visual Studio Developer Command Prompt):cmdcl server.cpp /EHsc /link ws2_32.lib
cl client1.cpp /EHsc /link ws2_32.lib
cl client2.cpp /EHsc /link ws2_32.lib

Usage
Running the Server
cmdserver.exe

Server starts on TCP port 54000.
HTTP monitoring starts automatically on port 8080.
Output will show: "Server running on port 54000" and "HTTP server running on port 8080".

Running Clients
Open multiple command prompts and run any of the client executables:
cmdclient1.exe
# or
client2.exe
# or
client3.exe

Type messages and press Enter.
Special commands:
STATUS → Server replies with Albanian message confirming it is active.
BYE → Client disconnects gracefully.
exit (client-side only) → Closes the client.

Messages are logged in real-time to msg_logs.txt.

Note: Client code currently hardcodes server IP as 172.20.10.3. Change this to your server's IP (e.g., 127.0.0.1 for localhost testing) before compiling.
Viewing Statistics
Open a browser or use curl:
Bashcurl http://localhost:8080/stats
or visit http://localhost:8080/stats in a web browser.
API / Core Functions
Server Core

main(): Initializes Winsock, starts TCP server on 54000, launches HTTP thread, and accepts clients.
handleClient(SOCKET, sockaddr_in): Thread function for each client. Handles receive loop, logging, command processing, and cleanup.
httpServer(): Background thread providing JSON stats at /stats.

Supported Commands (Client → Server)

Any text → Echoed/logged with "Kerkese e panjohur." reply (Albanian for "Unknown request.")
STATUS → "Serveri eshte aktiv dhe duke funksionuar."
BYE → "Lidhja po mbyllet." + disconnect

Configuration
No external config files or environment variables.
Hardcoded constants (in server.cpp):

PORT = 54000
MAX_CLIENTS = 3
TIMEOUT_MS = 10000 (10-second receive timeout)
HTTP port = 8080

To change settings, edit the constants and recompile the server.
Clients hardcode SERVER_IP = "172.20.10.3" and PORT = 54000.
Project Structure
textTCP_SOCKET_PROGRAMMING_37/
├── server.cpp          # Main TCP + HTTP monitoring server
├── client1.cpp         # Interactive TCP client (incomplete includes in original)
├── client2.cpp         # Similar interactive TCP client (more complete includes)
├── client3.cpp         # Third client implementation (content not fully available)
├── msg_logs.txt        # Auto-generated message log (timestamp + IP + message)
└── README.md
No subdirectories. All files are in the root.
Limitations / Known Issues

Windows-only: Uses Winsock2 and Windows-specific functions (localtime_s, inet_ntop, etc.).
Hardcoded IP in all clients (172.20.10.3) — requires manual edit for different networks or localhost testing.
client1.cpp has incomplete #include directives (missing headers in source).
No authentication or encryption.
Limited to 3 clients; additional connections are rejected without queue.
HTTP server runs indefinitely and does not handle errors gracefully in all cases.
No graceful server shutdown (Ctrl+C may leave sockets open).
Message buffer limited to 1024 bytes.
Logging uses ios::app without explicit error checking on file writes.

Future Improvements

Make server IP/port configurable via command-line arguments or config file.
Cross-platform support (switch to POSIX sockets or use Boost.Asio).
Add client reconnection logic and better error handling.
Implement proper message broadcasting between clients.
Add authentication (simple username/password).
Improve HTTP monitoring with more endpoints (e.g., /clients, /logs).
Containerize with Docker for easier deployment.
Add unit tests for socket handling logic.

Contributing
Contributions are welcome. Please:

Fork the repository.
Create a feature branch.
Ensure code compiles cleanly on Windows.
Update this README if behavior changes.

License
This project is open source. No license file is present in the repository. The maintainer may wish to add an MIT or GPL license.

Project Status: Educational demonstration of TCP socket programming with monitoring capabilities.