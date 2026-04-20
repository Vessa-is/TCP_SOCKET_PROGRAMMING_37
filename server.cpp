#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;

#pragma comment(lib, "ws2_32.lib")

using namespace std;

const int PORT = 54000;
const int MAX_CLIENTS = 3;
const int TIMEOUT_MS = 10000; 

atomic<int> activeClients(0);
vector<string> messageLog;
mutex logMutex;
vector<string> clientIPs;
atomic<int> totalMessages(0);
atomic<bool> adminAssigned(false);

string getCurrentTime() {
    time_t now = time(0);
    tm localTime;
    localtime_s(&localTime, &now);

    stringstream ss;
    ss << put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}
void handleClient(SOCKET clientSocket, sockaddr_in clientAddr, bool isAdmin) {
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO,
        (const char*)&TIMEOUT_MS, sizeof(TIMEOUT_MS));

    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);

    cout << "Client connected: " << clientIP << endl;
    if (isAdmin)
    cout << "→ This client is ADMIN\n";
else
    cout << "→ This client is READ-ONLY\n";
    {
    lock_guard<mutex> lock(logMutex);
    clientIPs.push_back(clientIP);
}

    char buffer[1024];

    while (true) {
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

        if (bytesReceived == SOCKET_ERROR) {
            int err = WSAGetLastError();

        if (err == WSAETIMEDOUT) {
            cout << "Client timed out (no messages): " << clientIP << endl;
        } else {
            cout << "Recv error from " << clientIP << endl;
        }
        break;
}
        if (bytesReceived <= 0) {
            cout << "Client disconnected: " << clientIP << endl;
            if (isAdmin) {
            cout << "\n=== ADMIN CONNECTED ===\n";
            cout << "Commands:\n";
            cout << "/list\n/read <file>\n/upload <file>\n/download <file>\n/delete <file>\n/search <keyword>\n/info <file>\nSTATUS\nBYE\n\n";
        } else {
            cout << "\n=== USER CONNECTED ===\n";
        }
            break;
        }

        buffer[bytesReceived] = '\0';
        totalMessages++;
        cout << "[" << clientIP << "]: " << buffer << endl;
    {
        lock_guard<mutex> lock(logMutex);
        string logEntry = "[" + getCurrentTime() + "] " + clientIP + ": " + buffer;
        messageLog.push_back(logEntry);
        ofstream logFile("msg_logs.txt", ios::app);
        logFile << logEntry << endl;
    }
        string request = buffer;
string reply;

// trim input
request.erase(request.find_last_not_of(" \n\r\t") + 1);

// ---------------- ALWAYS ALLOWED ----------------
if (request == "STATUS") {
    reply = "Serveri eshte aktiv dhe duke funksionuar.";
}
else if (request == "BYE") {
    reply = "Lidhja po mbyllet.";
    send(clientSocket, reply.c_str(), reply.size(), 0);
    break;
}

// ---------------- ADMIN ONLY ----------------
else {

    if (!isAdmin) {
        reply = "Access denied. Admin only.";
        send(clientSocket, reply.c_str(), reply.size(), 0);
        continue;
    }

    // -------- LIST --------
    if (request == "/list") {
        string result = "Files:\n";
        for (const auto& entry : fs::directory_iterator("server_storage")) {
            result += entry.path().filename().string() + "\n";
        }
        reply = result;
    }

    // -------- READ --------
    else if (request.rfind("/read ", 0) == 0) {
        string filename = request.substr(6);
        string path = "server_storage/" + filename;

        ifstream file(path);
        if (!file) {
            reply = "File not found.";
        } else {
            string line, content;
            while (getline(file, line)) content += line + "\n";
            reply = content;
        }
    }

    // -------- DELETE --------
    else if (request.rfind("/delete ", 0) == 0) {
        string filename = request.substr(8);
        string path = "server_storage/" + filename;

        if (remove(path.c_str()) == 0)
            reply = "File deleted.";
        else
            reply = "Delete failed.";
    }

    // -------- SEARCH --------
    else if (request.rfind("/search ", 0) == 0) {
        string keyword = request.substr(8);
        string result = "Matching files:\n";

        for (const auto& entry : fs::directory_iterator("server_storage")) {
            string name = entry.path().filename().string();
            if (name.find(keyword) != string::npos)
                result += name + "\n";
        }

        reply = result;
    }

    // -------- INFO --------
    else if (request.rfind("/info ", 0) == 0) {
        string filename = request.substr(6);
        string path = "server_storage/" + filename;

        if (!fs::exists(path)) {
            reply = "File not found.";
        } else {
            auto size = fs::file_size(path);
            reply = "Size: " + to_string(size) + " bytes";
        }
    }

    // -------- BINARY UPLOAD --------
    else if (request.rfind("/upload ", 0) == 0) {

        string filename = request.substr(8);
        string path = "server_storage/" + filename;

        ofstream file(path, ios::binary);

        size_t fileSize;
        recv(clientSocket, (char*)&fileSize, sizeof(fileSize), 0);

        char buffer[1024];
        size_t received = 0;

        while (received < fileSize) {
            int bytes = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytes <= 0) break;

            file.write(buffer, bytes);
            received += bytes;
        }

        file.close();

        reply = "File uploaded.";
        send(clientSocket, reply.c_str(), reply.size(), 0);
        continue;
    }

    // -------- BINARY DOWNLOAD --------
    else if (request.rfind("/download ", 0) == 0) {

        string filename = request.substr(10);
        string path = "server_storage/" + filename;

        ifstream file(path, ios::binary);
        if (!file) {
            reply = "File not found.";
            send(clientSocket, reply.c_str(), reply.size(), 0);
        } else {

            file.seekg(0, ios::end);
            size_t size = file.tellg();
            file.seekg(0);

            send(clientSocket, (char*)&size, sizeof(size), 0);

            char buffer[1024];
            while (!file.eof()) {
                file.read(buffer, sizeof(buffer));
                send(clientSocket, buffer, file.gcount(), 0);
            }

            file.close();
        }

        continue;
    }

    else {
        reply = "Unknown command.";
    }
}

send(clientSocket, reply.c_str(), reply.size(), 0);
send(clientSocket, reply.c_str(), reply.size(), 0);
send(clientSocket, reply.c_str(), reply.size(), 0);
    }

    {
    lock_guard<mutex> lock(logMutex);
    clientIPs.erase(remove(clientIPs.begin(), clientIPs.end(), clientIP), clientIPs.end());
}
    closesocket(clientSocket);
    activeClients--;  
}
void httpServer() {
    SOCKET httpSocket;
    sockaddr_in serverAddr{}, clientAddr{};
    int clientSize = sizeof(clientAddr);

    httpSocket = socket(AF_INET, SOCK_STREAM, 0);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(httpSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(httpSocket, SOMAXCONN);

    cout << "HTTP server running on port 8080\n";

    while (true) {
        SOCKET client = accept(httpSocket, (sockaddr*)&clientAddr, &clientSize);

        char buffer[1024];
        int bytes = recv(client, buffer, sizeof(buffer) - 1, 0);

        if (bytes > 0) {
            buffer[bytes] = '\0';
            string request(buffer);

            if (request.find("GET /stats") != string::npos) {

                stringstream json;

                lock_guard<mutex> lock(logMutex);

                json << "{\n";
                json << "\"active_clients\": " << activeClients << ",\n";

                json << "\"client_ips\": [";
                for (size_t i = 0; i < clientIPs.size(); i++) {
                    json << "\"" << clientIPs[i] << "\"";
                    if (i < clientIPs.size() - 1) json << ",";
                }
                json << "],\n";

                json << "\"total_messages\": " << totalMessages << ",\n";

                json << "\"messages\": [";
                for (size_t i = 0; i < messageLog.size(); i++) {
                    json << "\"" << messageLog[i] << "\"";
                    if (i < messageLog.size() - 1) json << ",";
                }
                json << "]\n";

                json << "}";

                string response =
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: application/json\r\n"
                    "Connection: close\r\n\r\n" +
                    json.str();

                send(client, response.c_str(), response.size(), 0);
            }
        }

        closesocket(client);
    }
}

int main() {
    WSADATA wsa;
    SOCKET serverSocket;
    sockaddr_in serverAddr{}, clientAddr{};
    int clientSize = sizeof(clientAddr);

    WSAStartup(MAKEWORD(2, 2), &wsa);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, SOMAXCONN);

    cout << "Server running on port " << PORT << endl;
    thread httpThread(httpServer);
httpThread.detach();

    while (true) {
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientSize);

        if (clientSocket == INVALID_SOCKET) {
            cout << "Accept failed\n";
            continue;
        }


        if (activeClients >= MAX_CLIENTS) {
            cout << "Server full → rejecting client\n";

            const char* msg = "Server is full. Connection rejected.\n";
            send(clientSocket, msg, strlen(msg), 0);

            closesocket(clientSocket);
            continue;
        }

 
        activeClients++;

        bool isAdmin = false;

    if (!adminAssigned) {
        isAdmin = true;
        adminAssigned = true;
    }

thread t(handleClient, clientSocket, clientAddr, isAdmin);
        t.detach();
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}