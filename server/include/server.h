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
    char* nick_name;
    int socket;
    sockaddr_in addr;
} client_t;

typedef struct {
    char* IP;
    char* port;
    int socket;
    sockaddr_in addr;
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
           const char* port_server = "5000")
        : port_to_client(port_to_client), port_server(port_server), IP(IP)
    {
        std::cout << "IP: " << this->IP << "\n";
        std::cout << "port_to_client: " << this->port_to_client << "\n";
        std::cout << "port_server: " << this->port_server << "\n";
    }

    void start();

private:
    void listen_to_client();
    void connect_server();

    int initalize_server(addrinfo_t &addr, const char* Port);

    /**
     * Handle communication between server and client
     * @brief handle_client
     * @param socket
     */
    void handle_client(int* socket);
    void handshake_to_client(int &socket);
    void handshake_to_server(int &socket);

    /**
     * @brief update_server_info: to other server, when new server connection is created
     */
    void update_server_info(int &socket);

    /**
     * @brief update_client_info: to other server, when new cleint connection is created
     */
    void update_client_info(int &socket);

    void accept_client(int socket);

private:
    addrinfo_t lisToClient;
    addrinfo_t lisToServer;
    addrinfo_t connServer;
    const char* port_to_client;
    const char* port_server;
    const char* IP;
    std::vector<std::thread> vec;
    std::vector<client_t> clients;
    std::vector<server_t> servers;
    std::vector<std::pair<server_t, client_t> > routing_table;
    sockaddr_in client_addr;
    int server_fd = 0;      // socket for server as client
};
#endif
