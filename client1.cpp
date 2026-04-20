#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#include <fstream>
#include <sstream>

#pragma comment(lib, "ws2_32.lib")

using namespace std;


const char* SERVER_IP = "172.20.10.2";  
int PORT = 54000;

void receiveMessages(SOCKET sock) {
    char buffer[4096];

    while (true) {
        int bytesReceived = recv(sock, buffer, sizeof(buffer), 0);

        if (bytesReceived <= 0) {
            cout << "\nDisconnected from server\n";
            break;
        }

        string response(buffer, bytesReceived);

        // ================= HANDLE DOWNLOAD =================
        if (response.rfind("FILE ", 0) == 0) {
            stringstream ss(response);
            string tag, filename;
            int fileSize;

            ss >> tag >> filename >> fileSize;

            ofstream file(filename, ios::binary);
            if (!file) {
                cout << "Cannot create file\n";
                continue;
            }

            int receivedTotal = 0;

            while (receivedTotal < fileSize) {
                int chunk = recv(sock, buffer, sizeof(buffer), 0);
                if (chunk <= 0) break;

                file.write(buffer, chunk);
                receivedTotal += chunk;
            }

            file.close();
            cout << "Downloaded file: " << filename << "\n";
        }

        // ================= NORMAL TEXT =================
        else {
            cout << response;
        }
    }
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
thread recvThread(receiveMessages, sock);
recvThread.detach();

  
  while (true) {
    string msg;
    cout << "Enter message: ";
    getline(cin, msg);

    if (msg == "exit") break;
    // ================= HANDLE UPLOAD =================
if (msg.rfind("/upload", 0) == 0) {

    string filename = msg.substr(8); // remove "/upload "

    ifstream file(filename, ios::binary);
    if (!file) {
        cout << "File not found\n";
        continue;
    }

    file.seekg(0, ios::end);
    int fileSize = file.tellg();
    file.seekg(0, ios::beg);

    string header = "/upload " + filename + " " + to_string(fileSize) + "\n";
    send(sock, header.c_str(), header.size(), 0);

    char buffer[4096];
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
        send(sock, buffer, file.gcount(), 0);
    }

    file.close();

    cout << "Upload sent\n";
    continue;
}

// ================= NORMAL COMMANDS =================
else {
    send(sock, msg.c_str(), msg.size(), 0);
}

  }

    closesocket(sock);
    WSACleanup();
    return 0;
}