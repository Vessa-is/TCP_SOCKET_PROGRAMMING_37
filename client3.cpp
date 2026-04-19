#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <cstddef>

#pragma comment(lib, "ws2_32.lib")

using namespace std;


const char* SERVER_IP = "172.20.10.3";  
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

  
    while (true) {
    string msg;
    cout << "Enter message: ";
    getline(cin, msg);

    if (msg == "exit") break;

    send(sock, msg.c_str(), msg.size(), 0);
    if (msg.rfind("/upload ", 0) == 0) {

    string filename = msg.substr(8);

    ifstream file(filename, ios::binary);
    if (!file) {
        cout << "File not found locally.\n";
        continue;
    }

    file.seekg(0, ios::end);
    size_t size = file.tellg();
    file.seekg(0);

    // send size
    send(sock, (char*)&size, sizeof(size), 0);

    char buffer[1024];
    while (!file.eof()) {
        file.read(buffer, sizeof(buffer));
        send(sock, buffer, file.gcount(), 0);
    }

    file.close();
}

    char buffer[1024];
    int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (msg.rfind("/download ", 0) == 0) {

    string filename = msg.substr(10);

    size_t fileSize;
    recv(sock, (char*)&fileSize, sizeof(fileSize), 0);

    ofstream file("downloaded" + filename, ios::binary);

    char buffer[1024];
    size_t received = 0;

    while (received < fileSize) {
        int bytes = recv(sock, buffer, sizeof(buffer), 0);
        if (bytes <= 0) break;

        file.write(buffer, bytes);
        received += bytes;
    }

    file.close();

    cout << "File downloaded successfully.\n";
}
else {
    // NORMAL TEXT RESPONSE
    char buffer[1024];
    int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);

    if (bytesReceived <= 0) {
        cout << "Disconnected from server\n";
        break;
    }

    buffer[bytesReceived] = '\0';
    cout << "Server: " << buffer << endl;
}

    if (bytesReceived <= 0) {
        cout << "Disconnected from server\n";
        break;
    }

    buffer[bytesReceived] = '\0';
    cout << "Server: " << buffer << endl;
}
    closesocket(sock);
    WSACleanup();
    return 0;

}