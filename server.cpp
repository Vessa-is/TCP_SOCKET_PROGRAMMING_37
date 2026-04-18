#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

int main() {
    WSADATA wsa;
    SOCKET serverSocket = INVALID_SOCKET;
    SOCKET clientSocket = INVALID_SOCKET;
    sockaddr_in serverAddr = {};
    sockaddr_in clientAddr = {};
    int clientSize = sizeof(clientAddr);

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        cout << "WSAStartup failed: " << WSAGetLastError() << endl;
        return 1;
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        cout << "Socket creation failed: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(54000);
    serverAddr.sin_addr.s_addr = INADDR_ANY;   // Listen on all interfaces (better than inet_pton for 0.0.0.0)

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cout << "Bind failed: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }


    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        cout << "Listen failed: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    cout << "Server running on port 54000...\n";


    clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientSize);
    if (clientSocket == INVALID_SOCKET) {
        cout << "Accept failed: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

  
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
    cout << "Client connected from: " << clientIP << endl;

 
    char buffer[1024] = {};
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        cout << "Client says: " << buffer << endl;


        const char* reply = "Hello from server!";
        send(clientSocket, reply, strlen(reply), 0);
    }


    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();

    return 0;
}