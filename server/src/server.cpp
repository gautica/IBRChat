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
    accept_client();
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

void Server::update_server_info(int &socket, const char* msg)
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

void Server::update_client_info(int &socket)
{
    for (auto &it : servers) {
        if (it.socket != socket) {

        }
    }
}


void Server::pack_s2s_message(char* msg, Serv_to_Serv &conn)
{
    strcpy(msg, "sus");
    strcat(msg, conn.conn);
    strcat(msg, ";");
}

void Server::pack_s2s_messages(char* msg)
{
    strcpy(msg, "sus");
    strcat(msg, connections.at(0).conn);
    for (unsigned int i = 1; i < connections.size(); i++) {
        strcat(msg, ";");
        strcat(msg, connections.at(i).conn);
    }
}

void Server::unpack_message(char* msg)
{
    if (msg[2] == 'c') {    // new server-client connection

    } else if (msg[2] == 's') {     // new server-server connection
        //msg[0] = msg[1] = msg[2] = ' ';
        Serv_to_Serv conn;
        std::cout << "msg: " << msg << "\n";
        char* token = strtok(msg, ";");
        printf("token: %s\n", token);

        strcpy(conn.conn, token+3);
        printf("conn.conn: %s\n", conn.conn);
        connections.push_back(conn);
        
        token = strtok(NULL, ";");
        while(token != NULL) {
            std::cout << "unpack_message\n";
            printf("token: %s\n", token);
            strcpy(conn.conn, token);
            connections.push_back(conn);
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

void Server::accept_client()
{
    while (true) {
        read_fds = master;
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        }
        // Run through the existing connections looking for data to read
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

void Server::handle_recvMsg(int &sock, char* buf)
{
    if (buf[0] == 's') {         // message from other server
        if (buf[1] == 'h') {     // handeshake from server in form of "sh[IP]"
            server_t new_serv;
            substring(new_serv.ID, buf, 2, sizeof(new_serv.ID));
            new_serv.socket = sock;
            servers.push_back(new_serv);
            Serv_to_Serv conn;
            strcpy(conn.conn, this->ID);
            strcat(conn.conn, "-");
            strcat(conn.conn, new_serv.ID);
            connections.push_back(conn);

            // update to other server
            mutex.lock();
            memset(this->sendBuf, 0, sizeof(sendBuf));
            pack_s2s_message(sendBuf, conn);
            update_server_info(sock, sendBuf);

            // update all info to new server
            memset(this->sendBuf, 0, sizeof(sendBuf));
            pack_s2s_messages(sendBuf);
            send(sock, sendBuf, sizeof(sendBuf), 0);
            mutex.unlock();
            //...
        } else if (buf[1] == 'u') {      // update info from server "su[Update]"
            mutex.lock();
            memset(this->sendBuf, 0, sizeof(sendBuf));
            strcpy(sendBuf, buf);
            unpack_message(buf);

            // send Update to other connected server
            update_server_info(sock, sendBuf);
            mutex.unlock();

            /**
             *  Test Search Algorithm
             */
            printf("connections.size(): %lu\n", connections.size());
            if (server_fd == 0 && connections.size() >= 2) {
                Graph graph;
                graph.create_graph(connections);
                graph.start_search(this->ID, servers.back().ID);
                std::cout << graph.get_next_Hup() << "\n";
            }
        }
    } else if (buf[0] == 'c') {  // message from client
        if (buf[1] == 'h') { // handshake from client in form of "ch[nick_name]"

        }
    }
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


