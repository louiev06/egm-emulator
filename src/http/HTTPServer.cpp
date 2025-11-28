#include "simulator/Machine.h"
#include "simulator/Game.h"
#include "sas/SASConstants.h"
#include "http/HTTPServer.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <fstream>
#include <set>

// C++11 compatible string helper
static bool endsWith(const std::string& str, const std::string& suffix) {
    if (suffix.length() > str.length()) return false;
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

// Simple JSON helper (for minimal dependencies)
static std::string jsonEscape(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c;
        }
    }
    return result;
}

HTTPServer::HTTPServer(simulator::Machine* machine, int port)
    : machine_(machine)
    , port_(port)
    , serverSocket_(-1)
    , running_(false)
{
}

HTTPServer::~HTTPServer() {
    stop();
}

void HTTPServer::start() {
    if (running_) {
        return;
    }

    std::cout << "Starting HTTP server on port " << port_ << "..." << std::endl;

    // Create server socket
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        std::cerr << "Failed to create server socket" << std::endl;
        return;
    }

    // Set socket options
    int reuse = 1;
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        std::cerr << "Failed to set SO_REUSEADDR" << std::endl;
        close(serverSocket_);
        return;
    }

    // Bind socket
    struct sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);

    if (bind(serverSocket_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Failed to bind to port " << port_ << std::endl;
        close(serverSocket_);
        return;
    }

    // Listen
    if (listen(serverSocket_, 5) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
        close(serverSocket_);
        return;
    }

    std::cout << "HTTP server listening on port " << port_ << std::endl;
    std::cout << "GUI URL: http://localhost:" << port_ << "/index.html" << std::endl;

    running_ = true;
    serverThread_ = std::thread(&HTTPServer::serverThread, this);
}

void HTTPServer::stop() {
    if (!running_) {
        return;
    }

    std::cout << "Stopping HTTP server..." << std::endl;
    running_ = false;

    if (serverSocket_ >= 0) {
        close(serverSocket_);
        serverSocket_ = -1;
    }

    if (serverThread_.joinable()) {
        serverThread_.join();
    }

    std::cout << "HTTP server stopped" << std::endl;
}

void HTTPServer::serverThread() {
    while (running_) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(serverSocket_, &readSet);

        struct timeval timeout{};
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int result = select(serverSocket_ + 1, &readSet, nullptr, nullptr, &timeout);

        if (result < 0) {
            if (running_) {
                std::cerr << "Select error" << std::endl;
            }
            break;
        } else if (result == 0) {
            // Timeout, continue loop
            continue;
        }

        if (FD_ISSET(serverSocket_, &readSet)) {
            struct sockaddr_in clientAddr{};
            socklen_t clientLen = sizeof(clientAddr);

            int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientLen);
            if (clientSocket >= 0) {
                // Handle client in a separate thread (or could use thread pool)
                std::thread clientThread(&HTTPServer::handleClient, this, clientSocket);
                clientThread.detach();
            }
        }
    }
}

void HTTPServer::handleClient(int clientSocket) {
    char buffer[4096];
    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        std::string request(buffer);
        std::string response = handleRequest(request);

        send(clientSocket, response.c_str(), response.length(), 0);
    }

    close(clientSocket);
}

HTTPServer::HTTPRequest HTTPServer::parseRequest(const std::string& request) {
    HTTPRequest req;
    std::istringstream stream(request);
    std::string line;

    // Parse request line
    if (std::getline(stream, line)) {
        std::istringstream lineStream(line);
        lineStream >> req.method >> req.path;
    }

    // Parse headers
    while (std::getline(stream, line) && line != "\r") {
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 2);
            if (!value.empty() && value[value.length()-1] == '\r') {
                value = value.substr(0, value.length()-1);
            }
            req.headers[key] = value;
        }
    }

    // Parse body
    std::string body;
    while (std::getline(stream, line)) {
        body += line;
    }
    req.body = body;

    return req;
}

std::string HTTPServer::buildResponse(int statusCode, const std::string& contentType, const std::string& body) {
    std::ostringstream response;

    std::string statusText;
    switch (statusCode) {
        case 200: statusText = "OK"; break;
        case 404: statusText = "Not Found"; break;
        case 500: statusText = "Internal Server Error"; break;
        default: statusText = "Unknown";
    }

    response << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << body.length() << "\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;

    return response.str();
}

std::string HTTPServer::handleRequest(const std::string& request) {
    HTTPRequest req = parseRequest(request);

    // Logging disabled to reduce console noise
    // std::cout << req.method << " " << req.path << std::endl;

    // API endpoints
    if (req.method == "GET" && req.path == "/api/status") {
        return buildResponse(200, "application/json", handleGET_Status());
    }
    else if (req.method == "GET" && req.path == "/api/ip") {
        return buildResponse(200, "application/json", handleGET_IP());
    }
    else if (req.method == "GET" && req.path == "/api/denoms") {
        return buildResponse(200, "application/json", handleGET_Denoms());
    }
    else if (req.method == "GET" && req.path == "/api/exceptions") {
        return buildResponse(200, "application/json", handleGET_Exceptions());
    }
    else if (req.method == "POST" && req.path == "/api/play") {
        return buildResponse(200, "application/json", handlePOST_Play(req.body));
    }
    else if (req.method == "POST" && req.path == "/api/cashout") {
        return buildResponse(200, "application/json", handlePOST_Cashout(req.body));
    }
    else if (req.method == "POST" && req.path == "/api/denom") {
        return buildResponse(200, "application/json", handlePOST_Denom(req.body));
    }
    else if (req.method == "POST" && req.path == "/api/exception") {
        return buildResponse(200, "application/json", handlePOST_Exception(req.body));
    }
    else if (req.method == "POST" && req.path == "/api/billinsert") {
        return buildResponse(200, "application/json", handlePOST_BillInsert(req.body));
    }
    // Static files
    else if (req.method == "GET") {
        return handleStaticFile(req.path);
    }

    // Not found
    return buildResponse(404, "text/plain", "Not Found");
}

std::string HTTPServer::handleGET_Status() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    auto game = machine_->getCurrentGame();
    std::ostringstream json;
    json << "{"
         << "\"credits\":" << (machine_->getCredits() / 100.0) << ","
         << "\"winAmount\":0.00,"
         << "\"denom\":" << (game ? game->getDenom() : 0.01) << ","
         << "\"gameName\":\"" << (game ? jsonEscape(game->getGameName()) : "No Game") << "\","
         << "\"isPlaying\":false,"
         << "\"status\":\"Ready\""
         << "}";

    return json.str();
}

std::string HTTPServer::handleGET_IP() {
    std::string ip = getIPAddress();
    return "{\"ip\":\"" + jsonEscape(ip) + "\"}";
}

std::string HTTPServer::handleGET_Denoms() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    // Get all unique denominations from available games
    std::set<double> denomSet;
    auto games = machine_->getGames();
    for (const auto& game : games) {
        denomSet.insert(game->getDenom());
    }

    // Build JSON array
    std::ostringstream json;
    json << "{\"denoms\":[";
    bool first = true;
    for (double denom : denomSet) {
        if (!first) json << ",";
        json << denom;
        first = false;
    }
    json << "]}";

    return json.str();
}

std::string HTTPServer::handleGET_Exceptions() {
    // Return common SAS exceptions
    std::ostringstream json;
    json << "{\"exceptions\":[";
    json << "{\"code\":16,\"name\":\"Slot Door Open\"},";
    json << "{\"code\":17,\"name\":\"Drop Door Open\"},";
    json << "{\"code\":18,\"name\":\"Card Cage Open\"},";
    json << "{\"code\":19,\"name\":\"Cashbox Door Open\"},";
    json << "{\"code\":20,\"name\":\"Cashbox Removed\"},";
    json << "{\"code\":21,\"name\":\"Belly Door Open\"},";
    json << "{\"code\":23,\"name\":\"Bill Acceptor Failure\"},";
    json << "{\"code\":24,\"name\":\"Bill Acceptor Full\"},";
    json << "{\"code\":25,\"name\":\"Printer Failure\"},";
    json << "{\"code\":26,\"name\":\"Printer Paper Out\"},";
    json << "{\"code\":32,\"name\":\"RAM Error\"},";
    json << "{\"code\":33,\"name\":\"Low Battery\"},";
    json << "{\"code\":64,\"name\":\"Handpay Pending\"},";
    json << "{\"code\":81,\"name\":\"Game Tilt\"},";
    json << "{\"code\":82,\"name\":\"Power Off/On\"}";
    json << "]}";
    return json.str();
}

std::string HTTPServer::handlePOST_Play(const std::string& body) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    // Play the game (deducts bet and updates COIN_IN meter)
    int64_t playResult = machine_->playGameCredit();

    if (playResult == 0) {
        // Insufficient credits
        std::ostringstream json;
        json << "{"
             << "\"credits\":" << (machine_->getCredits() / 100.0) << ","
             << "\"winAmount\":0.00,"
             << "\"success\":false,"
             << "\"error\":\"Insufficient credits\""
             << "}";
        return json.str();
    }

    // Simulate game outcome (60% lose, 40% win)
    // Win amounts range from 2x to 10x the bet
    int64_t winAmount = 0;
    int outcome = rand() % 100;

    if (outcome < 40) {  // 40% chance to win
        // Win between 2x and 10x the bet
        auto game = machine_->getCurrentGame();
        double betAmount = game ? game->getDenom() : 0.01;
        int multiplier = 2 + (rand() % 9);  // 2x to 10x
        double winDollars = betAmount * multiplier;

        // Add winnings using addCoinOut() - this updates both credits and COIN_OUT meter
        machine_->addCoinOut(winDollars);
        winAmount = static_cast<int64_t>(winDollars * 100);  // Convert to cents for display

        machine_->GameWon();
    } else {
        // Lost - no winnings
        machine_->GameLost();
    }

    std::ostringstream json;
    json << "{"
         << "\"credits\":" << (machine_->getCredits() / 100.0) << ","
         << "\"winAmount\":" << (winAmount / 100.0) << ","
         << "\"success\":true"
         << "}";
    return json.str();
}

std::string HTTPServer::handlePOST_Cashout(const std::string& body) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    int64_t amount = machine_->getCredits();
    machine_->cashoutButton();  // Use machine's cashout method

    std::ostringstream json;
    json << "{"
         << "\"amount\":" << (amount / 100.0) << ","
         << "\"credits\":" << (machine_->getCredits() / 100.0) << ","
         << "\"success\":true"
         << "}";
    return json.str();
}

std::string HTTPServer::handlePOST_Denom(const std::string& body) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    // Parse JSON body for denom value
    // Simple parsing: look for "denom":value
    size_t pos = body.find("\"denom\":");
    if (pos != std::string::npos) {
        double denom = std::stod(body.substr(pos + 8));
        // Try to switch to a game with this denomination
        machine_->setCurrentGame(1, denom);  // Game number 1
    }

    auto game = machine_->getCurrentGame();
    std::ostringstream json;
    json << "{"
         << "\"denom\":" << (game ? game->getDenom() : 0.01) << ","
         << "\"success\":true"
         << "}";
    return json.str();
}

std::string HTTPServer::handlePOST_Exception(const std::string& body) {
    // Parse exception code
    size_t pos = body.find("\"code\":");
    if (pos != std::string::npos) {
        int code = std::stoi(body.substr(pos + 7));
        std::cout << "Exception triggered: 0x" << std::hex << code << std::dec << std::endl;
        // TODO: Trigger actual SAS exception
    }

    return "{\"success\":true}";
}

std::string HTTPServer::handlePOST_BillInsert(const std::string& body) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    // Parse amount
    size_t pos = body.find("\"amount\":");
    if (pos != std::string::npos) {
        double amount = std::stod(body.substr(pos + 9));

        // Add credits from bill
        machine_->addCredits(amount);

        // Update bill acceptor meters based on denomination
        int billValue = static_cast<int>(amount);
        switch (billValue) {
            case 1:
                machine_->incrementMeter(sas::SASConstants::METER_1_BILLS_ACCEPTED, 1);
                break;
            case 5:
                machine_->incrementMeter(sas::SASConstants::METER_5_BILLS_ACCEPTED, 1);
                break;
            case 10:
                machine_->incrementMeter(sas::SASConstants::METER_10_BILLS_ACCEPTED, 1);
                break;
            case 20:
                machine_->incrementMeter(sas::SASConstants::METER_20_BILLS_ACCEPTED, 1);
                break;
            case 50:
                machine_->incrementMeter(sas::SASConstants::METER_50_BILLS_ACCEPTED, 1);
                break;
            case 100:
                machine_->incrementMeter(sas::SASConstants::METER_100_BILLS_ACCEPTED, 1);
                break;
        }

        // Update credits from bill acceptor meter
        int64_t creditAmount = static_cast<int64_t>(amount / machine_->getAccountingDenom());
        machine_->incrementMeter(sas::SASConstants::METER_CRD_FR_BILL_ACCEPTOR, creditAmount);
    }

    std::ostringstream json;
    json << "{"
         << "\"credits\":" << (machine_->getCredits() / 100.0) << ","
         << "\"success\":true"
         << "}";
    return json.str();
}

std::string HTTPServer::handleStaticFile(const std::string& path) {
    std::string filePath = "/opt/ncompass/media";

    if (path == "/" || path == "/index.html") {
        filePath += "/index.html";
    } else {
        filePath += path;
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        return buildResponse(404, "text/plain", "File not found");
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::string mimeType = getMimeType(filePath);

    return buildResponse(200, mimeType, content);
}

std::string HTTPServer::getMimeType(const std::string& path) {
    if (endsWith(path, ".html")) return "text/html";
    if (endsWith(path, ".css")) return "text/css";
    if (endsWith(path, ".js")) return "application/javascript";
    if (endsWith(path, ".json")) return "application/json";
    if (endsWith(path, ".png")) return "image/png";
    if (endsWith(path, ".jpg") || endsWith(path, ".jpeg")) return "image/jpeg";
    if (endsWith(path, ".gif")) return "image/gif";
    return "text/plain";
}

std::string HTTPServer::getIPAddress() {
    struct ifaddrs *ifaddr, *ifa;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        return "127.0.0.1";
    }

    std::string result = "127.0.0.1";

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) continue;

        int family = ifa->ifa_addr->sa_family;

        if (family == AF_INET) {
            int s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                               host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);

            if (s == 0 && strcmp(ifa->ifa_name, "lo") != 0) {
                result = host;
                break;
            }
        }
    }

    freeifaddrs(ifaddr);
    return result;
}
