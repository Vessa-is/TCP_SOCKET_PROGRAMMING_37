#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <fstream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

const char* SERVER_IP = "172.20.10.3";
const int PORT = 54000;

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cout << "Connection failed\n";
        return 1;
    }

    cout << "Connected to server!\n";

    // 🔥 RECEIVE MENU PROPERLY
    char buffer[4096];
    int bytes;

    while ((bytes = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes] = '\0';
        cout << buffer;

        // stop after full menu arrives
        if (string(buffer).find("BYE") != string::npos)
            break;
    }

    cout << endl;

    while (true) {

        string msg;
        cout << "\nEnter command: ";
        getline(cin, msg);

        if (msg == "exit") break;

        // SEND COMMAND
        send(sock, msg.c_str(), msg.size(), 0);

        // ================= UPLOAD =================
        if (msg.rfind("/upload ", 0) == 0) {

            string filename = msg.substr(8);

            ifstream file(filename, ios::binary | ios::ate);
            if (!file) {
                cout << "File not found locally.\n";
                continue;
            }

            size_t fileSize = file.tellg();
            file.seekg(0);

            // send file size
            send(sock, (char*)&fileSize, sizeof(fileSize), 0);

            while (!file.eof()) {
                file.read(buffer, sizeof(buffer));
                send(sock, buffer, file.gcount(), 0);
            }

            file.close();
            cout << "Upload complete.\n";
            continue;
        }

        // ================= DOWNLOAD =================
        else if (msg.rfind("/download ", 0) == 0) {

            string filename = msg.substr(10);

            size_t fileSize = 0;
            recv(sock, (char*)&fileSize, sizeof(fileSize), 0);

            if (fileSize == 0) {
                cout << "File not found or access denied.\n";
                continue;
            }

            ofstream file("downloaded" + filename, ios::binary);

            size_t received = 0;

            while (received < fileSize) {
                int bytes = recv(sock, buffer, sizeof(buffer), 0);
                if (bytes <= 0) break;

                file.write(buffer, bytes);
                received += bytes;
            }

            file.close();

            if (received == fileSize)
                cout << "Download complete: downloaded" << filename << endl;
            else
                cout << "Download incomplete.\n";

            continue;
        }

        // ================= NORMAL RESPONSE =================
        else {

            int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);

            if (bytesReceived <= 0) {
                cout << "Disconnected from server\n";
                break;
            }

            buffer[bytesReceived] = '\0';
            cout << "Server:\n" << buffer << endl;
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}