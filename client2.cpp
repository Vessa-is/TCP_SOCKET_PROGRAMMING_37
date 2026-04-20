#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

using namespace std;


const char* SERVER_IP = "172.20.10.3";
int PORT = 54000;

string recvLine(SOCKET sock) {
    string result;
    char ch;

       while (true) {
        int bytes = recv(sock, &ch, 1, 0);
        if (bytes <= 0) return result.empty() ? "" : result;
        result += ch;
        if (ch == '\n' || result.size() > 2000) break;  // safety
    }

    return result;
}

int main() {
    WSADATA wsa;
    SOCKET sock;
    sockaddr_in serverAddr{};

    WSAStartup(MAKEWORD(2,2), &wsa);

    sock = socket(AF_INET, SOCK_STREAM, 0);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);


    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cout << "Connection failed\n";
        return 1;
    }

    cout << "Connected to server!\n";
    char buffer[1024];
int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);

if (bytesReceived > 0) {
    buffer[bytesReceived] = '\0';
    cout << "Server: " << buffer << endl;
}

  
    while (true) {
    string msg;
    cout << "Enter message: ";
    getline(cin, msg);

    if (msg == "exit") break;

    send(sock, msg.c_str(), msg.size(), 0);

   string response = recvLine(sock);

if (response.empty()) {
    cout << "Disconnected from server\n";
    break;
}

cout << "Server: " << response << endl;

    buffer[bytesReceived] = '\0';
    cout << "Server: " << buffer << endl;
}

    closesocket(sock);
    WSACleanup();
    return 0;
}