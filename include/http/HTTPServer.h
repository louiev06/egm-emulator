#ifndef HTTP_HTTPSERVER_H
#define HTTP_HTTPSERVER_H

#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <map>
#include <mutex>

// Forward declaration
namespace simulator {
    class Machine;
}

class HTTPServer {
public:
    HTTPServer(simulator::Machine* machine, int port = 8080);
    ~HTTPServer();

    // Start/stop server
    void start();
    void stop();
    bool isRunning() const { return running_; }

    // Get server info
    std::string getIPAddress();

private:
    // Server thread function
    void serverThread();

    // Request handling
    void handleClient(int clientSocket);
    std::string handleRequest(const std::string& request);

    // HTTP parsing
    struct HTTPRequest {
        std::string method;      // GET, POST, etc.
        std::string path;        // /api/status
        std::string body;        // POST body
        std::map<std::string, std::string> headers;
    };

    HTTPRequest parseRequest(const std::string& request);
    std::string buildResponse(int statusCode, const std::string& contentType, const std::string& body);

    // API endpoint handlers
    std::string handleGET_Status();
    std::string handleGET_IP();
    std::string handleGET_Denoms();
    std::string handleGET_Exceptions();
    std::string handleGET_Meters();
    std::string handlePOST_Play(const std::string& body);
    std::string handlePOST_Cashout(const std::string& body);
    std::string handlePOST_Denom(const std::string& body);
    std::string handlePOST_Exception(const std::string& body);
    std::string handlePOST_BillInsert(const std::string& body);
    std::string handlePOST_Reboot(const std::string& body);

    // Static file serving
    std::string handleStaticFile(const std::string& path);
    std::string getMimeType(const std::string& path);

    // Members
    simulator::Machine* machine_;
    int port_;
    int serverSocket_;
    std::thread serverThread_;
    std::atomic<bool> running_;
    std::recursive_mutex mutex_;
};

#endif // HTTP_HTTPSERVER_H
