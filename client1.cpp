#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <fstream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

const char* SERVER_IP = "172.20.10.3";   // Change to 127.0.0.1 for local testing
const int PORT = 54000;

int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        cout << "WSAStartup failed\n";
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        cout << "Socket creation failed: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cout << "Connection failed: " << WSAGetLastError() << endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    cout << "Connected to server!\n";

    // Optional: Receive welcome message if server sends one
    char initBuf[1024] = {0};
    int bytes = recv(sock, initBuf, sizeof(initBuf)-1, 0);
    if (bytes > 0) {
        initBuf[bytes] = '\0';
        cout << initBuf << endl;
    }

    while (true) {
        string msg;
        cout << "\nEnter command: ";
        getline(cin, msg);

        if (msg == "exit" || msg == "quit") break;

        // Send the command
        if (send(sock, msg.c_str(), (int)msg.size(), 0) == SOCKET_ERROR) {
            cout << "Send failed\n";
            break;
        }

        // === UPLOAD HANDLING ===
        if (msg.rfind("/upload ", 0) == 0) {
            string filename = msg.substr(8);
            ifstream file(filename, ios::binary | ios::ate);
            if (!file) {
                cout << "File not found locally.\n";
                continue;
            }

            size_t fileSize = file.tellg();
            file.seekg(0);

            // Send file size (correctly)
            send(sock, (char*)&fileSize, sizeof(fileSize), 0);

            char buffer[4096];
            while (file) {
                file.read(buffer, sizeof(buffer));
                streamsize bytesRead = file.gcount();
                if (bytesRead > 0) {
                    send(sock, buffer, (int)bytesRead, 0);
                }
            }
            file.close();
            cout << "Upload completed.\n";
            continue;   // Skip normal recv
        }

        // === DOWNLOAD HANDLING ===
        else if (msg.rfind("/download ", 0) == 0) {
            string filename = msg.substr(10);

            size_t fileSize = 0;
            if (recv(sock, (char*)&fileSize, sizeof(fileSize), 0) <= 0) {
                cout << "Failed to receive file size\n";
                continue;
            }

            if (fileSize == 0) {
                cout << "File not found or access denied on server.\n";
                continue;
            }

            ofstream file("downloaded_" + filename, ios::binary);
            if (!file) {
                cout << "Cannot create local file.\n";
                continue;
            }

            char buffer[4096];
            size_t received = 0;
            while (received < fileSize) {
                int bytes = recv(sock, buffer, sizeof(buffer), 0);
                if (bytes <= 0) break;
                file.write(buffer, bytes);
                received += bytes;
            }
            file.close();

            if (received == fileSize)
                cout << "File downloaded successfully as downloaded_" << filename << "\n";
            else
                cout << "Download incomplete.\n";
            continue;   // Skip normal text response
        }

        // === NORMAL TEXT COMMANDS (/list, /read, /info, /search, /delete, STATUS, etc.) ===
        char response[4096] = {0};
        int bytesReceived = recv(sock, response, sizeof(response) - 1, 0);

        if (bytesReceived <= 0) {
            cout << "Disconnected from server\n";
            break;
        }

        response[bytesReceived] = '\0';
        cout << "Server:\n" << response << endl;
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}