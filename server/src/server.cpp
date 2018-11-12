#include "server.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <arpa/inet.h>

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

    int socket = initalize_server(lisToClient, port_to_client);
    accept_client(socket);
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
        if (recv(server_fd, buf, BUFFER_SIZE, 0) <= 0) {
            perror("recv");
            std::cout << "Failed to recieve update Info from server\n";
        }
        update_server_info(server_fd);
        std::cout << buf << std::endl;
    }
}

void Server::handle_client(int* socket)
{
    int client_fd = *socket;
    free(socket);
    /*************************************************/
    /************** HandShake ************************/
    /*************************************************/
    handshake_to_client(client_fd);

    // TO DO !!!
    // Listen to clients and also other servers as client
    char buf[BUFFER_SIZE] = {0};

    while (true) {
        if (recv(client_fd, buf, BUFFER_SIZE, 0) <= 0) {
            perror("recv");
            std::cout << "Failed to recieve update Info from server\n";
        }
        update_server_info(client_fd);
        std::cout << buf << std::endl;
    }

}

void Server::update_server_info(int &socket)
{
    char buf[50] = "I have new server connection";
    for (auto &it : servers) {
        if (it.socket != socket) {      // send info to other child servers
            std::cout << "send info to other servers\n";
            send(it.socket, buf, strlen(buf), 0);
        }
    }
    // Send info to parent server
    if (server_fd != 0 && server_fd != socket) {
        send(server_fd, buf, strlen(buf), 0);
    }
}

void Server::update_client_info(int &socket)
{
    for (auto &it : servers) {
        if (it.socket != socket) {

        }
    }
}

void Server::handshake_to_client(int &socket)
{
    char hostname[200] = {0};
    hostname[199] = 0;
    gethostname(hostname, 199);
    char msgS[500] = "Hello, I am Server: ";
    strcat(msgS, hostname);

    if (send(socket, msgS, 50, 0) <= 0) {
        std::cerr << "Failed to send message from Server to Client\n";
    }
    std::cout << "Send Handshake out\n" << "\n";
    char msgC[500] = {0};
    if (recv(socket, msgC, 500, 0) <= 0) {
        std::cerr <<"Failed to receive messafe from Client\n";
    }
    std::cout << msgC << std::endl;

    int id = atoi(msgC);
    if (id == 0) {
        /***** add new server to Vector servers ************/
        char IP[INET6_ADDRSTRLEN];
        inet_ntop(client_addr.sin_family, &client_addr.sin_addr, IP, sizeof (IP));
        server_t server;
        server.IP = IP;
        server.socket = socket;
        server.addr = client_addr;
        servers.push_back(server);

        // information to other connected servers
        update_server_info(socket);

        std::cout << "Add a new Server with IP: " << IP << "\n";
    } else if (id == 1) {
        /********** add new client to Vector clients **************/
        client_t client;
        client.addr = client_addr;
        client.socket = socket;
        clients.push_back(client);
        std::cout << "Add a new Client\n";
    } else {
        std::cerr << "Error: not identified as SERVER or CLIENT\n";
    }

}

void Server::handshake_to_server(int &socket)
{
    char msgS[2] = "0";         // 0 --> ID for server
    if (send(socket, msgS, strlen(msgS), 0) <= 0) {
        std::cerr << "Failed to send message from Server to Server\n";
    }
    char msgC[500] = {0};
    if (recv(socket, msgC, 500, 0) <= 0) {
        std::cerr <<"Failed to receive message from Server\n";
    }
    std::cout << msgC << std::endl;
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

void Server::accept_client(int socket)
{
    while (true) {
        int* new_client = new int;
        socklen_t addr_len = sizeof(client_addr);
        if ((*new_client = accept(socket, (struct sockaddr*) &client_addr, &addr_len)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        vec.push_back(std::thread(&Server::handle_client, this, new_client));
    }
}

