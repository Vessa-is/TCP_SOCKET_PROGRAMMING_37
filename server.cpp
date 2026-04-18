#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#include <atomic>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

const int PORT = 54000;
const int MAX_CLIENTS = 3;

atomic<int> activeClients(0);

void handleClient(SOCKET clientSocket, sockaddr_in clientAddr) {
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);

    cout << "Client connected: " << clientIP << endl;

    char buffer[1024];

    while (true) {
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

        if (bytesReceived <= 0) {
            cout << "Client disconnected: " << clientIP << endl;
            break;
        }

        buffer[bytesReceived] = '\0';
        cout << "[" << clientIP << "]: " << buffer << endl;

        const char* reply = "Hello from server!";
        send(clientSocket, reply, strlen(reply), 0);
    }

    closesocket(clientSocket);
    activeClients--;  
}

int main() {
    WSADATA wsa;
    SOCKET serverSocket;
    sockaddr_in serverAddr{}, clientAddr{};
    int clientSize = sizeof(clientAddr);

    WSAStartup(MAKEWORD(2, 2), &wsa);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, SOMAXCONN);

    cout << "Server running on port " << PORT << endl;

    while (true) {
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientSize);

        if (clientSocket == INVALID_SOCKET) {
            cout << "Accept failed\n";
            continue;
        }


        if (activeClients >= MAX_CLIENTS) {
            cout << "Server full → rejecting client\n";

            const char* msg = "Server is full. Connection rejected.\n";
            send(clientSocket, msg, strlen(msg), 0);

            closesocket(clientSocket);
            continue;
        }

 
        activeClients++;

        thread t(handleClient, clientSocket, clientAddr);
        t.detach();
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}