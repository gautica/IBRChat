#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <thread>
#include <vector>
#include <mutex>
#include <iostream>
#include <string.h>

#define BUFFER_SIZE 1024

typedef struct {
    char nick_name[50] = {0};
    int socket;
    //sockaddr_in addr;
} client_t;

typedef struct {
    char ID[INET6_ADDRSTRLEN+7] = {0};      // ID = IP:Port
    //char* port;
    int socket;
    //sockaddr_in addr;
} server_t;

typedef struct {
    char conn[3*INET6_ADDRSTRLEN] = {0};
} Serv_to_Serv;

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

        strcpy(this->ID, this->IP);
        strcat(this->ID, ":");
        strcat(this->ID, this->port_to_client);
        FD_ZERO(&master);
        FD_ZERO(&read_fds);
    }

    void start();

private:
    void listen_to_client();
    void connect_server();

    int initalize_server(addrinfo_t &addr, const char* Port);

    void* get_in_addr(struct sockaddr* remote_addr);
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
    void update_server_info(int &socket, const char* msg);

    void pack_s2s_message(char* msg, Serv_to_Serv &conn);
    void pack_s2s_messages(char* msg);
    void pack_s2c_message();
    void pack_s2c_messages();
    void unpack_message(char* msg);
    /**
     * @brief update_client_info: to other server, when new cleint connection is created
     */
    void update_client_info(int &socket);

    void accept_client();

    void handle_recvMsg(int &sock, char* buf);

    void substring(char* dest, char* src, const unsigned int &start, const unsigned int &length);

private:
    addrinfo_t lisToClient;
    addrinfo_t lisToServer;
    addrinfo_t connServer;
    const char* port_to_client;
    const char* port_server;
    const char* IP;
    char ID[INET6_ADDRSTRLEN+7];        // ID = IP:Port
    std::vector<std::thread> vec;
    std::vector<client_t> clients;
    std::vector<server_t> servers;
    std::vector<std::pair<server_t, client_t> > routing_table;
    sockaddr_in client_addr;
    int server_fd = 0;      // socket for server as client
    std::vector<Serv_to_Serv> connections;

    fd_set master, read_fds;
    struct sockaddr_storage remote_addr;
    int new_client_fd;
    int listner_fd;
    int fdmax;
    char revBuf[BUFFER_SIZE] = {0};
    char sendBuf[BUFFER_SIZE] = {0};
    std::mutex mutex;
};
#endif
