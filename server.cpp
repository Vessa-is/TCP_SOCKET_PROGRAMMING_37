#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <thread>


#pragma comment(lib, "ws2_32.lib")

#define port 54000

void handleClient(SOCKET clientSocket) {

    char buffer[1024];


while(true) {
     int bytesReceived = recv(clientSocket, buffer, sizeof(buffer),0);

     if(bytesReceived<=0) {
        cout << "Client disconnected\n";
        break;
     }

     string msg(buffer,bytesReceived);
     cout << "Client: " << msg << endl;

     string reply = "Server  received: " + msg;
     send(clientSocket, reply.c_str(), reply.size(),0);
}
closesocket(clientSocket);
}


int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, 5);

    cout << "Server running on port " << PORT << endl;

    while (true) {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);

        cout << "New client connected\n";

        thread t(handleClient, clientSocket);
        t.detach();
    }

    closesocket(serverSocket);
    WSACleanup();
}