#include "server.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

void Server::start()
{
    std::thread listen_to_client(&Server::listen_to_client, this);

    std::thread listen_to_server(&Server::listen_to_server, this);

    std::thread connect_server(&Server::connect_server, this);

    connect_server.join();
    listen_to_client.join();
    listen_to_server.join();

}

void Server::listen_to_client()
{
    memset(&lisToClient.hints, 0, sizeof (lisToClient.hints));
    lisToClient.hints.ai_family = AF_UNSPEC;
    lisToClient.hints.ai_socktype = SOCK_STREAM;

    int socket = initalize_server(lisToClient, port_to_client);
    accept_client(socket);
}

void Server::listen_to_server()
{
    memset(&lisToServer.hints, 0, sizeof(lisToServer.hints));
    lisToServer.hints.ai_family = AF_UNSPEC;
    lisToServer.hints.ai_socktype = SOCK_STREAM;

    int socket = initalize_server(lisToServer, port_to_server);
    accept_server(socket);
}

void Server::connect_server()
{
    memset(&connServer.hints, 0, sizeof(connServer.hints));
    connServer.hints.ai_family = AF_UNSPEC;
    connServer.hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(IP, port_server, &connServer.hints, &connServer.addr_info);
    int client_fd;
    if ((client_fd = socket(connServer.addr_info->ai_family, connServer.addr_info->ai_socktype, connServer.addr_info->ai_protocol)) == 0) {
        perror("socket:");
        exit(EXIT_FAILURE);
    }
    //sockaddr_in serverAddr;
    if (connect(client_fd, connServer.addr_info->ai_addr, connServer.addr_info->ai_addrlen) == -1) {
        perror("connect:");
        //exit(EXIT_FAILURE);
        return;
    }

    /*************************************************/
    /************** HandShake ************************/
    /*************************************************/
    handshake_to_server(client_fd);

    // TO DO !!!

}

int Server::initalize_server(addrinfo_t &addr, const char* Port)
{
    int server_fd;
    mutex.lock();
    getaddrinfo("localhost", Port, &addr.hints, &addr.addr_info);


    // Creating socket file descriptor
    if ((server_fd = socket(addr.addr_info->ai_family, addr.addr_info->ai_socktype, addr.addr_info->ai_protocol)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    if (bind(server_fd, addr.addr_info->ai_addr, addr.addr_info->ai_addrlen)<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    mutex.unlock();
    return server_fd;
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
}

void Server::handle_server(int* socket)
{
    int server_fd = *socket;
    free(socket);
    /*************************************************/
    /************** HandShake ************************/
    /*************************************************/
    handshake_to_server(server_fd);

    // TO DO !!!

}

void Server::handshake_to_server(int &socket)
{
    char hostname[200] = {0};
    hostname[199] = 0;
    gethostname(hostname, 199);
    char msgS[500] = "Hello, I am Server: ";
    strcat(msgS, hostname);
    if (send(socket, msgS, strlen(msgS), 0) <= 0) {
        std::cerr << "Failed to send message from Server to Server\n";
    }
    char msgC[500] = {0};
    if (recv(socket, msgC, 500, 0) <= 0) {
        std::cerr <<"Failed to receive message from Server\n";
    }
    std::cout << msgC << std::endl;
}

void Server::accept_client(int socket)
{
    while (true) {
        int* new_client = new int;
        sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        if ((*new_client = accept(socket, (struct sockaddr*) &client_addr, &addr_len)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        vec.push_back(std::thread(&Server::handle_client, this, new_client));
    }
}

void Server::accept_server(int socket)
{
    while (true) {
        int* new_server = new int;
        //sockaddr_in server_addr;
        //socklen_t addr_len = sizeof(server_addr);
        if ((*new_server = accept(socket, lisToServer.addr_info->ai_addr, &lisToServer.addr_info->ai_addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        vec.push_back(std::thread(&Server::handle_server, this, new_server));
    }
}


