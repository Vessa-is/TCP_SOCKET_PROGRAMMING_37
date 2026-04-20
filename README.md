## Overview

This project implements a **TCP-based file management server** in C++ using Windows Sockets (Winsock2). The server listens on port 54000 and supports multiple concurrent clients via a thread-per-client model. The first client to connect is automatically designated as **ADMIN** and receives an extended command menu for file operations inside a `server_storage` directory. Subsequent clients are regular **USER**s with only basic commands.

### What the project does
- Accepts TCP connections and assigns roles (first client = ADMIN).
- Provides a text-based command interface with binary file upload/download support.
- Allows the admin to list, read, delete, search, and get info about files stored on the server.

### The problem it solves
Demonstrates core TCP socket programming concepts on Windows: connection handling, multi-threading, role-based command dispatching, and binary data transfer over sockets.

### Key features and capabilities
- Automatic ADMIN role assignment (using `std::atomic`).
- Per-client command menu sent on connection.
- Full file management for ADMIN (`/list`, `/read`, `/upload`, `/download`, `/delete`, `/search`, `/info`).
- Basic commands available to all clients (`STATUS`, `BYE`).
- Detached threads for concurrent client handling (no artificial connection limit).


## Architecture / Design

### High-level system design
- Single-threaded acceptor loop in `main()`.
- Each accepted client spawns a detached `std::thread` running `handleClient()`.
- Role assignment is global and permanent (first successful connection only).

### Key modules/components and their responsibilities
- **main()**: Winsock initialization, socket creation, bind, listen, and infinite accept loop.
- **handleClient(SOCKET, sockaddr_in)**: Per-client handler. Sends role-specific menu, processes commands in a loop, performs file I/O, and handles upload/download binary transfers.
- No shared state beyond the global `adminAssigned` atomic flag.

### Data flow
1. Client connects → server accepts and spawns handler thread.
2. Handler sends menu (ADMIN or USER).
3. Client sends command → server parses, executes (file ops if ADMIN), sends reply.
4. For `/upload`: client sends command → server receives file size → receives binary data.
5. For `/download`: server sends file size → sends binary data.
6. Connection ends on `BYE`, recv error, or client disconnect.

## Tech Stack

- **Language**: C++ (C++17 required for `<filesystem>`)
- **Networking**: Winsock2 (`winsock2.h`, `ws2tcpip.h`)
- **Libraries**:
  - Standard C++: `<iostream>`, `<thread>`, `<atomic>`, `<filesystem>`, `<fstream>`
  - Windows-specific: `#pragma comment(lib, "ws2_32.lib")`
- **Platform**: Windows only
- **Build tools**: Visual Studio or `cl.exe` (Visual Studio Developer Command Prompt)

No external dependencies or version pins are declared in the code.

## Installation

### Prerequisites
- Windows OS
- Visual Studio (recommended) or Visual Studio Build Tools with C++ desktop development workload
- Winsock2 (included in Windows SDK)

### Step-by-step setup
1. Clone the repository:
   ```bash
   git clone https://github.com/Vessa-is/TCP_SOCKET_PROGRAMMING_37.git
   cd TCP_SOCKET_PROGRAMMING_37

Manually create the storage directory (required by server):cmdmkdir server_storage
Compile the server:cmdcl server.cpp /EHsc /link ws2_32.lib
Compile clients (each has its own hardcoded server IP):cmdcl client1.cpp /EHsc /link ws2_32.lib
cl client2.cpp /EHsc /link ws2_32.lib
cl client3.cpp /EHsc /link ws2_32.lib

Pre-built binary: server.exe is included and can be used directly.
Usage

How to run the project

Start the server:cmdserver.exeOutput: Server running on port 54000
Run any client in separate command prompts (edit hardcoded IP in .cpp files first if needed):cmdclient1.exe
client2.exe
client3.exe

Example commands (after connecting)
All clients:

STATUS → Server replies "Server is running."
BYE → Disconnects gracefully

ADMIN only (first client):

/list → Lists files in server_storage
/read <filename> → Displays file content
/delete <filename> → Deletes file
/search <keyword> → Lists files containing keyword in name
/info <filename> → Shows file size in bytes
/upload <filename> → Uploads local file to server (binary)
/download <filename> → Downloads file from server (binary)

Input/Output example (ADMIN client):
textEnter message: /list
Files:
test.txt

Enter message: /upload myfile.txt
Upload complete.
API / Core Functions
Server (server.cpp)

void handleClient(SOCKET clientSocket, sockaddr_in clientAddr): Main client logic, role assignment, command parsing, file operations, and binary transfer.
int main(): Winsock startup, TCP socket setup, accept loop, and thread spawning.

Clients

client1.cpp: Interactive client with partial upload/download support (expects "FILE " prefix for downloads — not sent by server).
client2.cpp: Minimal interactive client (text only).
client3.cpp: Interactive client with upload/download logic (upload protocol matches server; download logic contains duplicated recv calls and may behave unpredictably).

No formal class-based API or REST endpoints.
Configuration
All settings are hardcoded in the source files:

const int PORT = 54000; (server and clients)
Server IP in clients:
client1.cpp: 172.20.10.2
client2.cpp & client3.cpp: 172.20.10.3


No environment variables, config files, or command-line flags.
To change settings, edit the constants and recompile.

Project Structure
textTCP_SOCKET_PROGRAMMING_37/
├── README.md              # Existing (outdated) documentation
├── server.cpp             # Main TCP server implementation
├── client1.cpp            # Client with partial file transfer support
├── client2.cpp            # Minimal text-only client
├── client3.cpp            # Client with upload/download (partially matching server)
├── server.exe             # Pre-compiled Windows executable
├── msg_logs.txt           # Empty/placeholder log file (not used by current code)
├── test.txt               # Sample/test file (not used by current code)
└── server_storage/        # Must be created manually for file operations
All files are in the repository root; no subdirectories.