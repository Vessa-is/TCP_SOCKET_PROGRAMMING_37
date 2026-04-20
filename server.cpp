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
#include <string>
#include <sstream>     
#include <windows.h> 

#pragma comment(lib, "ws2_32.lib")

using namespace std;

const int PORT = 54000;
const int MAX_CLIENTS = 3;
const int TIMEOUT_MS = 10000; 

atomic<int> activeClients(3);
vector<string> messageLog;
mutex logMutex;
vector<string> clientIPs;
atomic<int> totalMessages(0);
atomic<int> nextClientId(1);
mutex adminMutex;
int adminClientId = -1;

string getCurrentTime() {
    time_t now = time(0);
    tm localTime;
    localtime_s(&localTime, &now);

    stringstream ss;
    ss << put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}
bool assignAdminIfNeeded(int clientId) {
    lock_guard<mutex> lock(adminMutex);

    if (adminClientId == -1) {
        adminClientId = clientId;
        return true;
    }

    return false;
}

bool isAdmin(int clientId) {
    lock_guard<mutex> lock(adminMutex);
    return adminClientId == clientId;
}

void releaseAdmin(int clientId) {
    lock_guard<mutex> lock(adminMutex);

    if (adminClientId == clientId) {
        adminClientId = -1;
        cout << "Admin disconnected, role released" << endl;
    }
}
void handleClient(SOCKET clientSocket, sockaddr_in clientAddr) {
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO,
        (const char*)&TIMEOUT_MS, sizeof(TIMEOUT_MS));

    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);

    cout << "Client connected: " << clientIP << endl;
    {
    lock_guard<mutex> lock(logMutex);
    clientIPs.push_back(clientIP);
}
int clientId = nextClientId++;
bool admin = assignAdminIfNeeded(clientId);

if (admin) {
    cout << "Client " << clientIP << " is ADMIN" << endl;
    string roleMsg = "You are ADMIN\n";
    send(clientSocket, roleMsg.c_str(), roleMsg.size(), 0);
        Sleep(50);
       string adminMenu =
        "\n=== ADMIN MENU ===\n"
        "/list               - List files in server directory\n"
        "/read <filename>    - Read file content\n"
        "/upload <filename>  - Upload file to server\n"
        "/download <filename>- Download file from server\n"
        "/delete <filename>  - Delete file from server\n"
        "/search <keyword>   - Search files by keyword\n"
        "/info <filename>    - Show file info (size, created, modified)\n"
        "===================\n";

    send(clientSocket, adminMenu.c_str(), adminMenu.size(), 0);
} else {
    string roleMsg = "You are USER\n";
    send(clientSocket, roleMsg.c_str(), roleMsg.size(), 0);
    Sleep(50);
   
    string userMenu =
        "\n=== USER MENU ===\n"
        "STATUS      - Check server status\n"
        "READ        - Read available files\n"
        "BYE         - Disconnect manually\n"
        "================\n";

    send(clientSocket, userMenu.c_str(), userMenu.size(), 0);
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
        request.erase(remove(request.begin(), request.end(), '\r'), request.end());
        request.erase(remove(request.begin(), request.end(), '\n'), request.end());
        string reply;


    stringstream ss(request);
string action, arg;
ss >> action;
ss >> arg;

bool adminUser = isAdmin(clientId);

// ================= ADMIN COMMANDS =================
if (action == "/list") {
    if (!adminUser) {
        string msg = "ERROR: Admin only\n";
        send(clientSocket, msg.c_str(), msg.size(), 0);
        continue;
    }

    system("dir > temp.txt");
    ifstream file("temp.txt");

    string line, output;
    while (getline(file, line)) output += line + "\n";

    send(clientSocket, output.c_str(), output.size(), 0);
}
else if (action == "/upload") {
    if (!adminUser) {
        string msg = "ERROR: Admin only\n";
        send(clientSocket, msg.c_str(), msg.size(), 0);
        continue;
    }

    int fileSize;
    ss >> fileSize;

    string path = "server_storage/" + arg;
ofstream file(path, ios::binary);
    if (!file) {
        string msg = "ERROR: Cannot create file\n";
        send(clientSocket, msg.c_str(), msg.size(), 0);
        continue;
    }

    char buffer[4096];
    int receivedTotal = 0;

    while (receivedTotal < fileSize) {
        int chunk = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (chunk <= 0) break;

        file.write(buffer, chunk);
        receivedTotal += chunk;
    }

    file.close();

    string msg = "UPLOAD OK\n";
    send(clientSocket, msg.c_str(), msg.size(), 0);
}
else if (action == "/download") {
    if (!adminUser) {
        string msg = "ERROR: Admin only\n";
        send(clientSocket, msg.c_str(), msg.size(), 0);
        continue;
    }

        string path = "server_storage/" + arg;
ifstream file(path, ios::binary);
        if (!file) {
        string msg = "ERROR: File not found\n";
        send(clientSocket, msg.c_str(), msg.size(), 0);
        continue;
    }
    file.seekg(0, ios::end);
    int fileSize = file.tellg();

    file.seekg(0, ios::beg);
    string header = "FILE " + arg + " " + to_string(fileSize) + "\n";
    send(clientSocket, header.c_str(), header.size(), 0);

    char buffer[4096];
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
        send(clientSocket, buffer, file.gcount(), 0);
    }

    file.close();
}
else if (action == "/delete") {
    if (!adminUser) {
        string msg = "ERROR: Admin only\n";
        send(clientSocket, msg.c_str(), msg.size(), 0);
        continue;
    }

    string path = "server_storage/" + arg;
ifstream file(path, ios::binary);
    remove(arg.c_str());
    string msg = "Deleted\n";
    send(clientSocket, msg.c_str(), msg.size(), 0);
}
else if (action == "/search") {
    if (!adminUser) {
        string msg = "ERROR: Admin only\n";
        send(clientSocket, msg.c_str(), msg.size(), 0);
        continue;
    }

    string cmd = "dir | findstr " + arg + " > search.txt";
    system(cmd.c_str());

    string path = "server_storage/" + arg;
ifstream file(path, ios::binary);
    string result((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());

    send(clientSocket, result.c_str(), result.size(), 0);
}
else if (action == "/info") {
    if (!adminUser) {
        string msg = "ERROR: Admin only\n";
        send(clientSocket, msg.c_str(), msg.size(), 0);
        continue;
    }

    WIN32_FILE_ATTRIBUTE_DATA info;
    if (GetFileAttributesExA(arg.c_str(), GetFileExInfoStandard, &info)) {
        string msg = "File info OK\n";
        send(clientSocket, msg.c_str(), msg.size(), 0);
    } else {
        string msg = "File not found\n";
        send(clientSocket, msg.c_str(), msg.size(), 0);
    }
}

// ================= USER + ADMIN =================
else if (action == "/read") {
    string path = "server_storage/" + arg;
ifstream file(path, ios::binary);
    if (!file) {
        string msg = "File not found\n";
        send(clientSocket, msg.c_str(), msg.size(), 0);
        continue;
    }

    string line, content;
    while (getline(file, line)) content += line + "\n";

    send(clientSocket, content.c_str(), content.size(), 0);
}
else if (action == "STATUS") {
    string msg = "Serveri eshte aktiv dhe duke funksionuar.\n";
    send(clientSocket, msg.c_str(), msg.size(), 0);
}
else if (action == "BYE") {
    string msg = "Lidhja po mbyllet.\n";
    send(clientSocket, msg.c_str(), msg.size(), 0);
    break;
}
else {
    string msg = "Kerkese e panjohur.\n";
    send(clientSocket, msg.c_str(), msg.size(), 0);
}

     
}
{
    lock_guard<mutex> lock(logMutex);
    clientIPs.erase(remove(clientIPs.begin(), clientIPs.end(), clientIP), clientIPs.end());
}
releaseAdmin(clientId);
    closesocket(clientSocket);
    activeClients--; 
}
void httpServer() {
    SOCKET httpSocket;
    sockaddr_in serverAddr{}, clientAddr{};
    int clientSize = sizeof(clientAddr);

    httpSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (httpSocket == INVALID_SOCKET) {
        cout << "HTTP socket creation failed: " << WSAGetLastError() << endl;
        return;
    }

    int opt = 1;
    setsockopt(httpSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(httpSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cout << "HTTP bind failed: " << WSAGetLastError() << endl;
        closesocket(httpSocket);
        return;
    }

    if (listen(httpSocket, SOMAXCONN) == SOCKET_ERROR) {
        cout << "HTTP listen failed: " << WSAGetLastError() << endl;
        closesocket(httpSocket);
        return;
    }

    cout << "HTTP server running on port 8080" << endl;

    while (true) {
        SOCKET client = accept(httpSocket, (sockaddr*)&clientAddr, &clientSize);
        if (client == INVALID_SOCKET) {
            cout << "HTTP accept failed: " << WSAGetLastError() << endl;
            continue;
        }

        char buffer[2048];
        int bytes = recv(client, buffer, sizeof(buffer) - 1, 0);

        if (bytes > 0) {
            buffer[bytes] = '\0';
            string request(buffer);

            if (request.find("GET /stats") != string::npos) {
                stringstream json;

                lock_guard<mutex> lock(logMutex);

                json << "{\n";
                json << "  \"active_clients\": " << activeClients << ",\n";

                json << "  \"client_ips\": [";
                for (size_t i = 0; i < clientIPs.size(); i++) {
                    json << "\"" << clientIPs[i] << "\"";
                    if (i < clientIPs.size() - 1) json << ",";
                }
                json << "],\n";

                json << "  \"total_messages\": " << totalMessages << ",\n";

                json << "  \"messages\": [";
                for (size_t i = 0; i < messageLog.size(); i++) {
                    json << "\"" << messageLog[i] << "\"";
                    if (i < messageLog.size() - 1) json << ",";
                }
                json << "]\n";

                json << "}";

                string body = json.str();
                string response =
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: application/json\r\n"
                    "Content-Length: " + to_string(body.size()) + "\r\n"
                    "Connection: close\r\n\r\n" +
                    body;

                send(client, response.c_str(), response.size(), 0);
            } else {
                string body = "{ \"error\": \"Use GET /stats\" }";
                string response =
                    "HTTP/1.1 404 Not Found\r\n"
                    "Content-Type: application/json\r\n"
                    "Content-Length: " + to_string(body.size()) + "\r\n"
                    "Connection: close\r\n\r\n" +
                    body;

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

        thread t(handleClient, clientSocket, clientAddr);
        t.detach();
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}