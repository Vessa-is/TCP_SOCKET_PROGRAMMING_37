#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <mutex>
#include <fstream>
#include <sstream>
#include <ctime>
#include <algorithm>
#include <filesystem>

#pragma comment(lib, "ws2_32.lib")

using namespace std;
namespace fs = std::filesystem;

#define TCP_PORT 54000
#define HTTP_PORT 8080
#define MAX_CLIENTS 5
#define BUFFER_SIZE 4096
#define TIMEOUT_SECONDS 120

const string STORAGE_DIR = "server_storage";
const string LOG_FILE = "server_messages.log";

struct ClientInfo {
    SOCKET socketFd;
    string ip;
    string username;
    bool isAdmin;
    time_t lastActivity;
};

vector<ClientInfo> clients;
vector<string> messageLog;
mutex clientsMutex;
mutex logMutex;

string getCurrentTimeString() {
    time_t now = time(nullptr);
    tm localTime{};
    localtime_s(&localTime, &now);

    char buffer[64];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &localTime);
    return string(buffer);
}

void ensureStorageExists() {
    if (!fs::exists(STORAGE_DIR)) {
        fs::create_directory(STORAGE_DIR);
    }
}

void appendLog(const string& msg) {
    lock_guard<mutex> lock(logMutex);

    string finalMsg = "[" + getCurrentTimeString() + "] " + msg;
    messageLog.push_back(finalMsg);

    ofstream out(LOG_FILE, ios::app);
    if (out.is_open()) {
        out << finalMsg << "\n";
    }
}

bool sendString(SOCKET s, const string& msg) {
    int totalSent = 0;
    int toSend = (int)msg.size();

    while (totalSent < toSend) {
        int sent = send(s, msg.c_str() + totalSent, toSend - totalSent, 0);
        if (sent == SOCKET_ERROR) {
            return false;
        }
        totalSent += sent;
    }
    return true;
}

string trim(const string& s) {
    size_t start = s.find_first_not_of(" \r\n\t");
    size_t end = s.find_last_not_of(" \r\n\t");
    if (start == string::npos || end == string::npos) {
        return "";
    }
    return s.substr(start, end - start + 1);
}

bool startsWith(const string& text, const string& prefix) {
    return text.rfind(prefix, 0) == 0;
}

ClientInfo* findClientBySocket(SOCKET s) {
    for (auto& c : clients) {
        if (c.socketFd == s) {
            return &c;
        }
    }
    return nullptr;
}

void removeClient(SOCKET s) {
    lock_guard<mutex> lock(clientsMutex);

    clients.erase(
        remove_if(clients.begin(), clients.end(),
            [s](const ClientInfo& c) { return c.socketFd == s; }),
        clients.end()
    );
}

void broadcastMessage(const string& msg, SOCKET sender) {
    lock_guard<mutex> lock(clientsMutex);

    for (const auto& client : clients) {
        if (client.socketFd != sender) {
            sendString(client.socketFd, msg);
        }
    }
}

vector<string> listFilesInStorage() {
    vector<string> result;

    if (!fs::exists(STORAGE_DIR)) {
        return result;
    }

    for (const auto& entry : fs::directory_iterator(STORAGE_DIR)) {
        if (entry.is_regular_file()) {
            result.push_back(entry.path().filename().string());
        }
    }

    sort(result.begin(), result.end());
    return result;
}

string commandList() {
    vector<string> files = listFilesInStorage();
    stringstream ss;
    ss << "OK\n";
    for (const auto& f : files) {
        ss << f << "\n";
    }
    return ss.str();
}

string commandRead(const string& filename) {
    string path = STORAGE_DIR + "\\" + filename;

    ifstream in(path, ios::binary);
    if (!in.is_open()) {
        return "ERROR: File not found.\n";
    }

    stringstream buffer;
    buffer << in.rdbuf();

    return "OK\n" + buffer.str() + "\n";
}

string commandInfo(const string& filename) {
    fs::path p = fs::path(STORAGE_DIR) / filename;

    if (!fs::exists(p) || !fs::is_regular_file(p)) {
        return "ERROR: File not found.\n";
    }

    auto size = fs::file_size(p);
    auto ftime = fs::last_write_time(p);

    auto sctp = chrono::time_point_cast<chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() + chrono::system_clock::now()
    );
    time_t cftime = chrono::system_clock::to_time_t(sctp);

    tm localTime{};
    localtime_s(&localTime, &cftime);

    char timeBuffer[64];
    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &localTime);

    stringstream ss;
    ss << "OK\n";
    ss << "Name: " << filename << "\n";
    ss << "Size: " << size << " bytes\n";
    ss << "Last modified: " << timeBuffer << "\n";

    return ss.str();
}

string commandSearch(const string& keyword) {
    vector<string> files = listFilesInStorage();

    stringstream ss;
    ss << "OK\n";

    for (const auto& f : files) {
        if (f.find(keyword) != string::npos) {
            ss << f << "\n";
        }
    }

    return ss.str();
}

string commandDelete(const string& filename) {
    fs::path p = fs::path(STORAGE_DIR) / filename;

    if (!fs::exists(p)) {
        return "ERROR: File not found.\n";
    }

    if (fs::remove(p)) {
        return "OK: File deleted.\n";
    }

    return "ERROR: Could not delete file.\n";
}

string commandUploadSimple(const string& fullCommand) {

    size_t firstSpace = fullCommand.find(' ');
    if (firstSpace == string::npos) {
        return "ERROR: Usage /upload <filename>|<content>\n";
    }

    string rest = fullCommand.substr(firstSpace + 1);
    size_t pipePos = rest.find('|');
    if (pipePos == string::npos) {
        return "ERROR: Usage /upload <filename>|<content>\n";
    }

    string filename = trim(rest.substr(0, pipePos));
    string content = rest.substr(pipePos + 1);

    if (filename.empty()) {
        return "ERROR: Invalid filename.\n";
    }

    fs::path p = fs::path(STORAGE_DIR) / filename;

    ofstream out(p, ios::binary);
    if (!out.is_open()) {
        return "ERROR: Could not create file.\n";
    }

    out << content;
    out.close();

    return "OK: File uploaded.\n";
}

string commandDownload(const string& filename) {
    return commandRead(filename);
}

string buildStatsJson() {
    lock_guard<mutex> clientsLock(clientsMutex);
    lock_guard<mutex> logLock(logMutex);

    stringstream ss;
    ss << "{\n";
    ss << "  \"active_connections\": " << clients.size() << ",\n";
    ss << "  \"clients\": [\n";

    for (size_t i = 0; i < clients.size(); ++i) {
        ss << "    {"
           << "\"username\": \"" << clients[i].username << "\", "
           << "\"ip\": \"" << clients[i].ip << "\", "
           << "\"role\": \"" << (clients[i].isAdmin ? "admin" : "user") << "\""
           << "}";
        if (i + 1 < clients.size()) {
            ss << ",";
        }
        ss << "\n";
    }

    ss << "  ],\n";
    ss << "  \"message_count\": " << messageLog.size() << ",\n";
    ss << "  \"messages\": [\n";

    size_t start = messageLog.size() > 20 ? messageLog.size() - 20 : 0;
    for (size_t i = start; i < messageLog.size(); ++i) {
        string safeMsg = messageLog[i];
        replace(safeMsg.begin(), safeMsg.end(), '"', '\'');

        ss << "    \"" << safeMsg << "\"";
        if (i + 1 < messageLog.size()) {
            ss << ",";
        }
        ss << "\n";
    }

    ss << "  ]\n";
    ss << "}\n";

    return ss.str();
}

void httpServerThread() {
    SOCKET httpSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (httpSocket == INVALID_SOCKET) {
        cout << "[HTTP] Socket creation failed\n";
        return;
    }

    sockaddr_in httpAddr{};
    httpAddr.sin_family = AF_INET;
    httpAddr.sin_port = htons(HTTP_PORT);
    httpAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(httpSocket, (sockaddr*)&httpAddr, sizeof(httpAddr)) == SOCKET_ERROR) {
        cout << "[HTTP] Bind failed\n";
        closesocket(httpSocket);
        return;
    }

    if (listen(httpSocket, 10) == SOCKET_ERROR) {
        cout << "[HTTP] Listen failed\n";
        closesocket(httpSocket);
        return;
    }

    cout << "[HTTP] Monitoring server running on port " << HTTP_PORT << endl;

    while (true) {
        SOCKET client = accept(httpSocket, nullptr, nullptr);
        if (client == INVALID_SOCKET) {
            continue;
        }

        char buffer[BUFFER_SIZE];
        ZeroMemory(buffer, sizeof(buffer));

        int bytesReceived = recv(client, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0) {
            string request(buffer, bytesReceived);
            string responseBody;
            string statusLine;

            if (startsWith(request, "GET /stats")) {
                responseBody = buildStatsJson();
                statusLine = "HTTP/1.1 200 OK\r\n";
            } else {
                responseBody = "{\"error\":\"Not found\"}\n";
                statusLine = "HTTP/1.1 404 Not Found\r\n";
            }

            stringstream response;
            response << statusLine
                     << "Content-Type: application/json\r\n"
                     << "Content-Length: " << responseBody.size() << "\r\n"
                     << "Connection: close\r\n\r\n"
                     << responseBody;

            sendString(client, response.str());
        }

        closesocket(client);
    }
}

void timeoutMonitorThread() {
    while (true) {
        Sleep(5000);

        vector<SOCKET> toDisconnect;
        time_t now = time(nullptr);

        {
            lock_guard<mutex> lock(clientsMutex);

            for (const auto& c : clients) {
                if (difftime(now, c.lastActivity) > TIMEOUT_SECONDS) {
                    toDisconnect.push_back(c.socketFd);
                }
            }
        }

        for (SOCKET s : toDisconnect) {
            appendLog("Client timed out and was disconnected.");
            shutdown(s, SD_BOTH);
            closesocket(s);
            removeClient(s);
        }
    }
}

string processCommand(const string& rawRequest, ClientInfo& client) {
    string request = trim(rawRequest);

    if (request.empty()) {
        return "ERROR: Empty request.\n";
    }


    if (!client.isAdmin) {
        Sleep(150);
    }

    if (startsWith(request, "/msg ")) {
        string text = request.substr(5);
        string formatted = "[" + client.username + "] " + text + "\n";

        appendLog(client.username + ": " + text);
        broadcastMessage(formatted, client.socketFd);

        return "OK: Message broadcasted.\n";
    }

    if (request == "/list") {
        appendLog(client.username + " executed /list");
        return commandList();
    }

    if (startsWith(request, "/read ")) {
        string filename = trim(request.substr(6));
        appendLog(client.username + " executed /read " + filename);
        return commandRead(filename);
    }

    if (startsWith(request, "/search ")) {
        string keyword = trim(request.substr(8));
        appendLog(client.username + " executed /search " + keyword);
        return commandSearch(keyword);
    }

    if (startsWith(request, "/info ")) {
        string filename = trim(request.substr(6));
        appendLog(client.username + " executed /info " + filename);
        return commandInfo(filename);
    }

    if (startsWith(request, "/download ")) {
        string filename = trim(request.substr(10));
        appendLog(client.username + " executed /download " + filename);
        return commandDownload(filename);
    }

    if (startsWith(request, "/upload ")) {
        if (!client.isAdmin) {
            return "ERROR: Permission denied. Admin only.\n";
        }
        appendLog(client.username + " executed " + request);
        return commandUploadSimple(request);
    }

    if (startsWith(request, "/delete ")) {
        if (!client.isAdmin) {
            return "ERROR: Permission denied. Admin only.\n";
        }
        string filename = trim(request.substr(8));
        appendLog(client.username + " executed /delete " + filename);
        return commandDelete(filename);
    }

    if (request == "/whoami") {
        stringstream ss;
        ss << "OK\n";
        ss << "Username: " << client.username << "\n";
        ss << "Role: " << (client.isAdmin ? "admin" : "user") << "\n";
        ss << "IP: " << client.ip << "\n";
        return ss.str();
    }

    return "ERROR: Unknown command.\n";
}

void handleClientThread(SOCKET clientSocket, string clientIp) {
    char buffer[BUFFER_SIZE];
    ZeroMemory(buffer, sizeof(buffer));

    int firstRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (firstRead <= 0) {
        closesocket(clientSocket);
        return;
    }

    string firstMsg(buffer, firstRead);
    firstMsg = trim(firstMsg);

    string username = firstMsg.empty() ? "user" : firstMsg;
    bool isAdmin = false;

    {
        lock_guard<mutex> lock(clientsMutex);

      
        isAdmin = clients.empty();

        clients.push_back({ clientSocket, clientIp, username, isAdmin, time(nullptr) });
    }

    appendLog(username + " connected from " + clientIp + (isAdmin ? " as admin" : " as user"));

    if (isAdmin) {
        sendString(clientSocket, "OK: Connected as ADMIN.\n");
    } else {
        sendString(clientSocket, "OK: Connected as USER.\n");
    }

    while (true) {
        ZeroMemory(buffer, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

        if (bytesReceived <= 0) {
            break;
        }

        string request(buffer, bytesReceived);

        {
            lock_guard<mutex> lock(clientsMutex);
            ClientInfo* c = findClientBySocket(clientSocket);
            if (c) {
                c->lastActivity = time(nullptr);
            }
        }

        string response;
        {
            lock_guard<mutex> lock(clientsMutex);
            ClientInfo* c = findClientBySocket(clientSocket);
            if (!c) {
                response = "ERROR: Client not found.\n";
            } else {
                response = processCommand(request, *c);
            }
        }

        if (!sendString(clientSocket, response)) {
            break;
        }
    }

    appendLog(username + " disconnected.");
    removeClient(clientSocket);
    closesocket(clientSocket);
}

int main() {
    ensureStorageExists();

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "WSAStartup failed\n";
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        cout << "Socket creation failed\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(TCP_PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cout << "Bind failed\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, MAX_CLIENTS) == SOCKET_ERROR) {
        cout << "Listen failed\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    cout << "[TCP] Server running on port " << TCP_PORT << endl;
    cout << "[TCP] Max clients: " << MAX_CLIENTS << endl;

    thread httpThread(httpServerThread);
    httpThread.detach();

    thread timeoutThread(timeoutMonitorThread);
    timeoutThread.detach();

    while (true) {
        sockaddr_in clientAddr{};
        int clientLen = sizeof(clientAddr);

        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientLen);
        if (clientSocket == INVALID_SOCKET) {
            continue;
        }

        char ipBuffer[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, ipBuffer, INET_ADDRSTRLEN);
        string clientIp = ipBuffer;

        {
            lock_guard<mutex> lock(clientsMutex);
            if ((int)clients.size() >= MAX_CLIENTS) {
                sendString(clientSocket, "ERROR: Server full. Try again later.\n");
                closesocket(clientSocket);
                continue;
            }
        }

        cout << "[TCP] New incoming connection from " << clientIp << endl;

        thread clientThread(handleClientThread, clientSocket, clientIp);
        clientThread.detach();
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}