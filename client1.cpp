#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

#define PORT 54000

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);

    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cout << "Connection failed\n";
        return 1;
    }

    cout << "Connected to server\n";

    char buffer[1024];

    while (true) {
        string msg;

        cout << "Enter message: ";
        getline(cin, msg);

        send(clientSocket, msg.c_str(), msg.size(), 0);

        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesReceived <= 0) {
            cout << "Server disconnected\n";
            break;
        }

        string response(buffer, bytesReceived);
        cout << "Server: " << response << endl;
    }

    closesocket(clientSocket);
    WSACleanup();
}