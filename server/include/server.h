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
#define NICK_SIZE 50
#define CHANNEL_SIZE 50
#define CHANNEL_THEMA_SIZE 500
#define CMD_SIZE 2

typedef struct {
    char nick[NICK_SIZE] = {0};
    int socket;
} client_t;

typedef struct {
    char ID[INET6_ADDRSTRLEN+7] = {0};      // ID = IP:Port
    int socket;
} server_t;

typedef struct {
    char ID[CHANNEL_SIZE] = {0};
    char creator[NICK_SIZE] = {0};
    char thema[CHANNEL_THEMA_SIZE] = {0};
    std::vector<client_t> clients;
} channel_t;

typedef struct {
    char conn[3*INET6_ADDRSTRLEN] = {0};
} s2s_t;

typedef struct {
    char conn[INET6_ADDRSTRLEN + NICK_SIZE + 2] = {0};
} s2c_t;

typedef struct {
    char conn[CHANNEL_SIZE+NICK_SIZE+2] = {0};
} ch2c_t;

typedef struct {
    struct addrinfo hints;
    addrinfo* addr_info;
} addrinfo_t;

enum ERROR {
    UNVALID_NICK,
    UNVALID_CLIENT
};

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
        FD_ZERO(&server_fds);
        FD_ZERO(&client_fds);
    }

    void start();

private:
    void listen_to_client();
    void connect_server();

    int initalize_server(addrinfo_t &addr, const char* Port);

    void* get_in_addr(struct sockaddr* remote_addr);

    /**
     *  remove client/server, if client/server disconnected
     */
    void handle_disconnected(int &sock);

    /**
     * send 0 when username is valid, 1 if not
     */
    bool is_nick_valid(char *nick);

    /**
     * Search matching only by other servers
     * return true, if client is in channel, otherweise false
     */
    bool client_match_channel(char *client, char *channel);

    /**
     * @brief get_fd(): return fd, if client_name is under this server and the channel, otherweise -1
     */
    int get_fd(char *client_name, char* channel);

    /**
     * @brief get_server(char *client_name, char *server): get server ID, if client is under other server
     * return true, if server is found and saved in server
     * return false, if nicht found
     */
    bool get_server(char *client_name, char *server);

    /**
     * return fd of next connected server, which has a path from server to dest_server
     * return -1, otherweise
     */
    int get_next_hop(char *dest_server);

    /**
     * @brief update_info: to other server, if there are infos to update
     */
    void update_info(int &socket, char* msg);

    void pack_cmd(unsigned int cmd, char* buf);
    void pack_s2s_message(char* msg, s2s_t &conn);
    void pack_s2s_messages(char* msg);
    void pack_s2c_message(char *msg, s2c_t &conn);
    void pack_ch2c_message(char* msg, ch2c_t &conn);

    void add_new_client(char* buf);
    void remove_client(char* buf);
    void add_new_server(char* buf);
    void add_new_ch2c(char* buf);

    /**
     * @brief unpack_message: unpack update_info between servers
     */
    void unpack_update_info(char* msg);

    void accept_connection();

    //void handle_recvMsg(int &sock, char* buf);
    void handshake_to_server(int &socket);
    void handshake_to_client(int &socket);

    void handle_handshake(int &sock, char* buf);
    void handle_server_fds();
    void handle_client_fds();

    void handle_server_handshake(int &sock, char *buf);
    void handle_server_update(int &sock, char *buf);
    void handle_client_handshake(int &sock, char* buf);
    void handle_client_update(int &sock, char*buf);

    /**
     * Handle client chat message
     * @brief handle_client
     * @param socket
     */
    void handle_client(int &socket, char *buf);
    void handle_errors(int &sock, int ERROR);

    /***************************************************/
    /************ Handle Command ***********************/
    /***************************************************/
    /**
     * @brief join(): can failed, if cleint is already in a channel
     */
    bool join(int &sock, char* buf);
    bool leave(int &sock);
    bool change_nick(int &sock, char* buf);
    void list_channels(int &sock);
    void get_topic(int &sock, char *topic);
    bool set_topic(int &sock, char *topic);
    void send_to_one(int &sock, char *buf);
    void send_to_all(int &sock, char *buf);
    void quit(int &sock);

    void substring(char* dest, char* src, const unsigned int &start, const unsigned int &length);
    void send_data(int &sock, char* buf);

private:
    addrinfo_t lisToClient;
    addrinfo_t lisToServer;
    addrinfo_t connServer;
    const char* port_to_client;
    const char* port_server;
    const char* IP;
    char ID[INET6_ADDRSTRLEN+7];        // ID = IP:Port
    char parentID[INET6_ADDRSTRLEN+7];
    std::vector<client_t> clients;
    std::vector<server_t> servers;
    std::vector<channel_t> channels;
    std::vector<s2s_t> s2s_connections;
    std::vector<s2c_t> s2c_connections;
    std::vector<ch2c_t> ch2c_connections;

    fd_set master, read_fds, server_fds, client_fds;

    struct sockaddr_storage remote_addr;
    int new_client_fd;
    int server_fd = 0;
    int listner_fd;
    int fdmax;
    char revBuf[BUFFER_SIZE] = {0};
    char sendBuf[BUFFER_SIZE] = {0};

    std::mutex mutex;
};
#endif
