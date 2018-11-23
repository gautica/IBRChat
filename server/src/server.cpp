#include "server.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <arpa/inet.h>
#include "graph.h"

void Server::start()
{
    std::thread listen_to_client(&Server::listen_to_client, this);

    std::thread connect_server(&Server::connect_server, this);

    connect_server.join();
    listen_to_client.join();
}

void Server::listen_to_client()
{
    memset(&lisToClient.hints, 0, sizeof (lisToClient.hints));
    lisToClient.hints.ai_family = AF_UNSPEC;
    lisToClient.hints.ai_socktype = SOCK_STREAM;

    listner_fd = initalize_server(lisToClient, port_to_client);
    FD_SET(listner_fd, &master);
    fdmax = listner_fd;
    accept_connection();
}

void Server::accept_connection()
{
    while (true) {
        read_fds = master;
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        }
        // Run through the existing s2s_connections looking for data to read
        for (int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {       // Got one active socket
                if (i == listner_fd) {          // Got new connection
                   socklen_t addrlen = sizeof(remote_addr);
                   new_client_fd = accept(listner_fd, (struct sockaddr*) &remote_addr, &addrlen);
                   if (new_client_fd == -1) {
                       perror("accept");
                   } else {
                       FD_SET(new_client_fd, &master);
                       if (fdmax < new_client_fd) {
                           fdmax = new_client_fd;
                       }
                       char IP[INET6_ADDRSTRLEN];
                       printf("selectserver: new connection from %s on socket %d\n",
                              inet_ntop(remote_addr.ss_family, get_in_addr((struct sockaddr*)&remote_addr), IP, INET6_ADDRSTRLEN),
                              new_client_fd);
                   }
                } else {                        // Got message from one client
                    int nbyte = 0;
                    memset(revBuf, 0, sizeof (revBuf));     // clear buffer
                    if ((nbyte = recv(i, revBuf, BUFFER_SIZE, 0)) <= 0) {
                        if (nbyte == 0) {       // client has closed connection
                            printf("selectserver: socket %d hung up\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i);
                        FD_CLR(i, &master);
                    } else {        // get some Data from clients or servers
                        std::cout << "revBuf: " << revBuf << "\n";
                        handle_recvMsg(i, revBuf);
                    }
                }
            }
        }
    }
}

int Server::initalize_server(addrinfo_t &addr, const char* Port)
{
    int sock;
    getaddrinfo(IP, Port, &addr.hints, &addr.addr_info);

    // Creating socket file descriptor
    if ((sock = socket(addr.addr_info->ai_family, addr.addr_info->ai_socktype, addr.addr_info->ai_protocol)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    if (bind(sock, addr.addr_info->ai_addr, addr.addr_info->ai_addrlen)<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(sock, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    return sock;
}

void Server::connect_server()
{
    memset(&connServer.hints, 0, sizeof(connServer.hints));
    connServer.hints.ai_family = AF_UNSPEC;
    connServer.hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(IP, port_server, &connServer.hints, &connServer.addr_info);

    if ((server_fd = socket(connServer.addr_info->ai_family, connServer.addr_info->ai_socktype, connServer.addr_info->ai_protocol)) == 0) {
        perror("socket:");
        exit(EXIT_FAILURE);
    }
    //sockaddr_in serverAddr;
    if (connect(server_fd, connServer.addr_info->ai_addr, connServer.addr_info->ai_addrlen) == -1) {
        perror("connect:");
        std::cout << "##############################################\n";
        std::cout << "Attention: Fault only not first started Server\n";
        std::cout << "##############################################\n";
        //exit(EXIT_FAILURE);
        server_fd = 0;
        return;
    }

    /*************************************************/
    /************** HandShake ************************/
    /*************************************************/
    handshake_to_server(server_fd);

    // TO DO !!!
    char buf[BUFFER_SIZE] = {0};
    while (true) {
        if (recv(server_fd, buf, sizeof(buf), 0) <= 0) {
            //perror("recv");
            //std::cout << "Failed to recieve update Info from server\n";
        } else {
            handle_recvMsg(server_fd, buf);
            std::cout << buf << std::endl;
            memset(buf, 0, sizeof(buf));    // clear buffer
        }
    }
}

void Server::update_info(int &socket, const char* msg)
{
    for (auto &it : servers) {
        if (it.socket != socket) {      // send info to other child servers
            std::cout << "send info to other servers\n";
            send(it.socket, msg, strlen(msg), 0);
        }
    }
    // Send info to parent server
    if (server_fd != 0 && server_fd != socket) {
        send(server_fd, msg, strlen(msg), 0);
    }
}

void Server::pack_s2s_message(char* msg, s2s_t &conn)
{
    strcpy(msg, "sus");
    strcat(msg, conn.conn);
    strcat(msg, ";");
}

void Server::pack_s2s_messages(char* msg)
{
    strcpy(msg, "sus");
    strcat(msg, s2s_connections.at(0).conn);
    for (unsigned int i = 1; i < s2s_connections.size(); i++) {
        strcat(msg, ";");
        strcat(msg, s2s_connections.at(i).conn);
    }
}

void Server::pack_s2c_message(char *msg, s2c_t &conn)
{
    strcpy(msg, "suc");
    strcat(msg, conn.conn);
}

void Server::unpack_update_info(char* msg)
{
    if (msg[2] == 'c') {    // new server-client connection
        s2c_t conn;
        strcpy(conn.conn, msg+3);
        s2c_connections.push_back(conn);

    } else if (msg[2] == 's') {     // new server-server connection
        //msg[0] = msg[1] = msg[2] = ' ';
        s2s_t conn;
        char* token = strtok(msg, ";");
        printf("token: %s\n", token);
        strcpy(conn.conn, token+3);
        printf("conn.conn: %s\n", conn.conn);
        s2s_connections.push_back(conn);
        
        token = strtok(NULL, ";");
        while(token != NULL) {
            std::cout << "unpack_update_info\n";
            printf("token: %s\n", token);
            strcpy(conn.conn, token);
            s2s_connections.push_back(conn);
            token = strtok(NULL, ";");
        }
    }
}

void Server::handshake_to_server(int &socket)
{
    char msgS[BUFFER_SIZE] = "sh";       // sh -> from "server" "handshake"
    strcat(msgS, this->ID);             // Form "sc[ID]"
    if (send(socket, msgS, strlen(msgS), 0) <= 0) {
        std::cerr << "Failed to send message from Server to Server\n";
    }
    char msgC[BUFFER_SIZE] = {0};
    if (recv(socket, msgC, sizeof (msgC), 0) <= 0) {
        std::cerr <<"Failed to receive message from Server\n";
    }
    std::cout << msgC << std::endl;
    handle_recvMsg(socket, msgC);
    
}

void Server::handshake_to_client(int &socket)
{
    char msg[BUFFER_SIZE] = "Hello, I'm server with IP: ";
    strcat(msg, this->ID);
    if (send(socket, msg, strlen(msg), 0) <= 0) {
        std::cerr << "Failed to send handshake to new client\n";
    }
}

void Server::handle_recvMsg(int &sock, char* buf)
{
    if (buf[0] == 's') {         // message from other server
        if (buf[1] == 'h') {     // handeshake from server in form of "sh[IP]"
            handle_server_handshake(sock, buf);
        } else if (buf[1] == 'u') {      // update info from server "su[Update]"
            handle_server_update(sock, buf);
        }
    } else if (buf[0] == 'c') {  // message from client
        if (buf[1] == 'h') { // handshake from client in form of "ch[nick_name]"
            handle_client_handshake(sock, buf);
        } else if (buf[1] == 'c') { // client chat message
            handle_client(sock, buf);
        }
    }
}

void Server::handle_server_handshake(int &sock, char* buf)
{
    server_t new_serv;
    substring(new_serv.ID, buf, 2, sizeof(new_serv.ID));
    new_serv.socket = sock;
    servers.push_back(new_serv);
    s2s_t conn;
    strcpy(conn.conn, this->ID);
    strcat(conn.conn, "-");
    strcat(conn.conn, new_serv.ID);
    s2s_connections.push_back(conn);

    // update to other server
    mutex.lock();
    memset(this->sendBuf, 0, sizeof(sendBuf));
    pack_s2s_message(sendBuf, conn);
    update_info(sock, sendBuf);

    // update all info to new server
    memset(this->sendBuf, 0, sizeof(sendBuf));
    pack_s2s_messages(sendBuf);
    send(sock, sendBuf, sizeof(sendBuf), 0);
    mutex.unlock();
}

void Server::handle_server_update(int &sock,  char* buf)
{
    mutex.lock();
    memset(this->sendBuf, 0, sizeof(sendBuf));
    strcpy(sendBuf, buf);
    // send Update to other connected server
    update_info(sock, sendBuf);
    mutex.unlock();

    unpack_update_info(buf);
}

void Server::handle_client_handshake(int &sock, char* buf)
{
    client_t new_client;
    new_client.socket = sock;
    strcpy(new_client.nick, buf+2);
    clients.push_back(new_client);

    // update to other server
    s2c_t conn;
    strcpy(conn.conn, this->ID);
    strcat(conn.conn, "-");
    strcat(conn.conn, new_client.nick);

    mutex.lock();
    memset(sendBuf, 0, sizeof(sendBuf));
    pack_s2c_message(sendBuf, conn);
    update_info(sock, sendBuf);
    mutex.unlock();

    handshake_to_client(sock);
}

void Server::handle_client_update(int &sock, char *buf)
{
    s2c_t conn;
    
    mutex.lock();
    memset(sendBuf, 0, sizeof(sendBuf));
    strcpy(sendBuf, buf);
    // update other servers
    update_info(sock, sendBuf);
    mutex.unlock();

    unpack_update_info(buf);
}

void Server::handle_client(int &sock, char *buf)
{
    if (buf[2] == 'o') {        
        send_to_one(buf);
    } else if (buf[2] == 'm') {
        send_to_all(sock, buf);
    }
}

bool Server::client_match_channel(char *client, char *channel)
{
    for (auto &it : ch2c_connections) {
        if (strstr(it.conn, channel) != NULL && strstr(it.conn, client) != NULL) {
            return true;
        }
    }
    return false;
}

int Server::get_fd(char *client_name, char *ch_name)
{
    
    for (auto &channel : channels) {
        if (strcmp(channel.ID, ch_name) == 0) {
            for (auto &client: channel.clients) {
                if (strcmp(client_name, client.nick) == 0) {
                    return client.socket;
                }
            }
        }
    }
    return -1;
}

bool Server::get_server(char *client_name, char *server)
{
    for (auto &it : s2c_connections) {
        if (strstr(it.conn, client_name) != NULL) {
            server = strtok(it.conn, "-");
            return true;
        }
    }
    return false;
}

int Server::get_next_hop(char *dest_server)
{
    Graph graph;
    graph.create_graph(s2s_connections);
    graph.start_search(this->ID, dest_server);
    char* server = graph.get_next_Hop();
    for (auto &it : servers) {
        if (strcmp(server, it.ID) == 0) {
            return it.socket;
        }
    }
    return -1;
}

void Server::send_to_one(char *buf)
{
    char name[NICK_SIZE] = {0};
    char cp_buf[sizeof(buf)];
    strcpy(cp_buf, buf);
    
    char *token = strtok(cp_buf, ":");
    strcpy(name, token+3);

    char *channel = strtok(NULL, ":");

    int fd = get_fd(name, channel);
    if (fd != -1) {
        token = strtok(NULL, ":");
        if (send(fd, token, strlen(token), 0) <= 0) {   // send message direct to client
            std::cout << "Failed to send to client " << name << "\n";
        }
    } else {
        char server[INET6_ADDRSTRLEN] = {0};
        if (client_match_channel(name, channel)) {
            if (get_server(name, server)) {
                fd = get_next_hop(server);
                if (fd != -1) {
                    if (send(fd, buf, strlen(buf), 0) <= 0) {   // send message direct to client
                        std::cout << "Failed to send to server " << server << "\n";
                    }
                } else {
                    std::cerr << "Cannot find next hop\n";
                }
            } else {
                // !!!
                std::cerr << "Cannot find client mit name " << name << "\n";
            }
        } else {
            std::cerr << "Client [" << name << "] do not under channel [" << channel << "]" << "\n";
        }        
    }
}

void Server::send_to_all(int &sock, char *buf)
{
    // Send to all other servers
    for (auto &it : servers) {
        if (it.socket != sock) {
            if (send(it.socket, buf, strlen(buf), 0) <= 0) {
                std::cerr << "Failed to send to server: " << it.ID << "\n";
            }
        }
    }

    // Send to all clients who are under same channel
    char *token = strtok(buf, ":");
    char channel[CHANNEL_SIZE];
    strcpy(channel, token+3);
    char *msg = strtok(NULL, ":");
    for (auto &it : channels) {
        if (strcmp(it.ID, channel) == 0) {
            for (auto &client : it.clients) {
                if (send(client.socket, msg, sizeof(msg), 0) <= 0) {
                    perror("send");
                    std::cerr << "Failed to send message to client [" << client.nick << "]" << "\n";
                }
            }
            return;
        }
    }
}

bool Server::join(char* client_name, char *channel)
{
    bool is_success = false;
    bool valid_ch = false;
    int index;
    // find this client from this->clients vector
    for (unsigned int i = 0; i < this->clients.size(); i++) {
        if (strcmp(clients.at(i).nick, client_name) == 0) {
            index = i;
            is_success = true;
        } 
    }
    // add this client to channel->clients vector
    if (is_success) {
        for (auto &it : channels) {
           if (strcmp(it.ID, channel) == 0) {
               it.clients.push_back(clients.at(index));
               clients.erase(clients.begin() + index);
               valid_ch = true;
               break;
           }
        }
        if (!valid_ch) {
            channel_t ch;
            strcpy(ch.ID, channel);
            strcpy(ch.creator, client_name);
            ch.clients.push_back(clients.at(index));
            clients.erase(clients.begin() + index);
            channels.push_back(ch);
        }
    }
    return is_success;
}

bool Server::leave(char* client_name, char *channel)
{
    for (auto &it : clients) {
        if (strcmp(it.nick, client_name) == 0) {
            return false;
        }
    }
    for (auto &it : channels) {
        for (unsigned int i = 0; i < it.clients.size(); i++) {
            if(strcmp(it.clients.at(i).nick, client_name) == 0) {
                clients.push_back(it.clients.at(i));
                it.clients.erase(it.clients.begin() + i);
                return true;
            }
        }
    }
    return false;
}

void Server::substring(char* dest, char* src, const unsigned int &start, const unsigned int &length)
{
    for (unsigned int i = start; i < start + length; i++) {
        dest[i-start] = src[i];
    }
}

void* Server::get_in_addr(struct sockaddr* sa)
{
    if (sa->sa_family == AF_INET) {
            return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


