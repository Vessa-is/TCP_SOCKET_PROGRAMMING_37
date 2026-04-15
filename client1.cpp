#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>

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

    string username;
    cout << "Enter username: ";
    getline(cin, username);

    send(clientSocket, username.c_str(), username.size(), 0);

    char buffer[4096];

    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived > 0) {
        string response(buffer, bytesReceived);
        cout << response << endl;
    }

    cout << "\nAvailable commands:\n";
    cout << "/msg <text>\n";
    cout << "/list\n";
    cout << "/read <file>\n";
    cout << "/search <keyword>\n";
    cout << "/info <file>\n";
    cout << "/download <file>\n";
    cout << "/upload <file>|<content> (admin only)\n";
    cout << "/delete <file> (admin only)\n";
    cout << "/whoami\n\n";

    while (true) {
        string command;

        cout << "> ";
        getline(cin, command);

        if (command.empty()) continue;

        send(clientSocket, command.c_str(), command.size(), 0);

        ZeroMemory(buffer, sizeof(buffer));

        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesReceived <= 0) {
            cout << "Disconnected from server\n";
            break;
        }

        string response(buffer, bytesReceived);
        cout << response << endl;
    }

    closesocket(clientSocket);
    WSACleanup();
}
