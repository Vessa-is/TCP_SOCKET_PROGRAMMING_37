#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <filesystem>
#include <fstream>

#pragma comment(lib, "ws2_32.lib")

using namespace std;
namespace fs = std::filesystem;

const int PORT = 54000;

atomic<bool> adminAssigned(false);

void handleClient(SOCKET clientSocket, sockaddr_in clientAddr) {

    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);

    bool isAdmin = false;

    if (!adminAssigned) {
        isAdmin = true;
        adminAssigned = true;
    }

    cout << "Client connected: " << clientIP 
         << (isAdmin ? " [ADMIN]" : " [USER]") << endl;

    // 🔥 SEND MENU TO CLIENT
    string menu;

    if (isAdmin) {
        menu =
            "\n=== ADMIN MENU ===\n"
            "/list\n/read <file>\n/upload <file>\n/download <file>\n"
            "/delete <file>\n/search <keyword>\n/info <file>\nSTATUS\nBYE\n\n";
    } else {
        menu =
            "\n=== USER MENU ===\n"
            "STATUS\nBYE\n\n";
    }

    send(clientSocket, menu.c_str(), menu.size(), 0);

    char buffer[4096];

    while (true) {

        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) break;

        buffer[bytesReceived] = '\0';
        string request = buffer;

        // trim
        request.erase(request.find_last_not_of(" \n\r\t") + 1);

        string reply;

        // ---------------- ALWAYS ALLOWED ----------------
        if (request == "STATUS") {
            reply = "Server is running.";
        }

        else if (request == "BYE") {
            reply = "Disconnecting...";
            send(clientSocket, reply.c_str(), reply.size(), 0);
            break;
        }

        // ---------------- USER BLOCK ----------------
        else if (!isAdmin) {
            reply = "Invalid request. Only STATUS and BYE allowed.";
        }

        // ---------------- ADMIN COMMANDS ----------------
        else if (request == "/list") {

            string result = "Files:\n";
            for (auto& entry : fs::directory_iterator("server_storage")) {
                result += entry.path().filename().string() + "\n";
            }
            reply = result;
        }

        else if (request.rfind("/read ", 0) == 0) {

            string filename = request.substr(6);
            ifstream file("server_storage/" + filename);

            if (!file) reply = "File not found.";
            else {
                string line, content;
                while (getline(file, line)) content += line + "\n";
                reply = content;
            }
        }

        else if (request.rfind("/delete ", 0) == 0) {

            string filename = request.substr(8);
            string path = "server_storage/" + filename;

            if (remove(path.c_str()) == 0)
                reply = "File deleted.";
            else
                reply = "Delete failed.";
        }

        else if (request.rfind("/search ", 0) == 0) {

            string keyword = request.substr(8);
            string result = "Matching files:\n";

            for (auto& entry : fs::directory_iterator("server_storage")) {
                string name = entry.path().filename().string();
                if (name.find(keyword) != string::npos)
                    result += name + "\n";
            }

            reply = result;
        }

        else if (request.rfind("/info ", 0) == 0) {

            string filename = request.substr(6);
            string path = "server_storage/" + filename;

            if (!fs::exists(path))
                reply = "File not found.";
            else {
                auto size = fs::file_size(path);
                reply = "Size: " + to_string(size) + " bytes";
            }
        }

        // ---------------- UPLOAD ----------------
        else if (request.rfind("/upload ", 0) == 0) {

            string filename = request.substr(8);
            ofstream file("server_storage/" + filename, ios::binary);

            size_t fileSize;
            recv(clientSocket, (char*)&fileSize, sizeof(fileSize), 0);

            size_t received = 0;
            while (received < fileSize) {
                int bytes = recv(clientSocket, buffer, sizeof(buffer), 0);
                if (bytes <= 0) break;

                file.write(buffer, bytes);
                received += bytes;
            }

            file.close();
            reply = "Upload complete.";
        }

        // ---------------- DOWNLOAD ----------------
        else if (request.rfind("/download ", 0) == 0) {

            string filename = request.substr(10);
            ifstream file("server_storage/" + filename, ios::binary);

            if (!file) {
                size_t zero = 0;
                send(clientSocket, (char*)&zero, sizeof(zero), 0);
                continue;
            }

            file.seekg(0, ios::end);
            size_t size = file.tellg();
            file.seekg(0);

            send(clientSocket, (char*)&size, sizeof(size), 0);

            while (!file.eof()) {
                file.read(buffer, sizeof(buffer));
                send(clientSocket, buffer, file.gcount(), 0);
            }

            file.close();
            continue;
        }

        // ---------------- UNKNOWN ----------------
        else {
            reply = "Unknown command.";
        }

        send(clientSocket, reply.c_str(), reply.size(), 0);
    }

    closesocket(clientSocket);
}

int main() {

    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, SOMAXCONN);

    cout << "Server running on port " << PORT << endl;

    while (true) {

        sockaddr_in clientAddr{};
        int clientSize = sizeof(clientAddr);

        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientSize);

        thread t(handleClient, clientSocket, clientAddr);
        t.detach();
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}