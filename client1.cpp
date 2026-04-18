#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

using namespace std;


const char* SERVER_IP = "192.168.50.144";  
int PORT = 54000;

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

  
    string msg = "Hello from client!";
    send(sock, msg.c_str(), msg.size(), 0);

    
    char buffer[1024];
    int bytesReceived = recv(sock, buffer, sizeof(buffer), 0);

    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        cout << "Server replied: " << buffer << endl;
    }

    closesocket(sock);
    WSACleanup();
}