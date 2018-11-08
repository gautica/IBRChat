#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <thread>
#include <vector>
#include <mutex>
#include <iostream>

#define BUFFER_SIZE 1024

typedef struct {
    char* IP;
    char* Port;
} cleint_t;

typedef struct {
    char* IP;
    char* Port;
} server_t;

typedef struct {
    struct addrinfo hints;
    addrinfo* addr_info;
} addrinfo_t;

class Server
{
public:
    /**
     * @brief Server
     * @param IP: ip of Server to connect
     * @param port_to_client
     * @param port_to_server
     * @param port_server: port to connect server
     */
    Server(const char* IP = "localhost",
           const char* port_to_client = "5000",
           const char* port_to_server = "6000",
           const char* port_server = "6000")
        : port_to_client(port_to_client), port_to_server(port_to_server), port_server(port_server), IP(IP)
    {
        std::cout << "IP: " << this->IP << "\n";
        std::cout << "port_to_client: " << this->port_to_client << "\n";
        std::cout << "port_to_server: " << this->port_to_server << "\n";
        std::cout << "port_server: " << this->port_server << "\n";
    }

    void start();

private:
    void listen_to_client();
    void listen_to_server();
    void connect_server();

    int initalize_server(addrinfo_t &addr, const char* Port);
    /**
     * Handle communication between server and client
     * @brief handle_client
     * @param socket
     */
    void handle_client(int* socket);
    void handshake_to_client(int &socket);
    /**
     * Hnadle communication between server and server
     * @brief handle_server
     * @param socket
     */
    void handle_server(int* socket);
    void handshake_to_server(int &socket);

    void accept_client(int socket);

    void accept_server(int socket);

private:
    addrinfo_t lisToClient;
    addrinfo_t lisToServer;
    addrinfo_t connServer;
    const char* port_to_client;
    const char* port_to_server;
    const char* port_server;
    const char* IP;
    std::mutex mutex;
    std::vector<std::thread> vec;
};
#endif
