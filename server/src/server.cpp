#include "server.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <arpa/inet.h>
#include <sys/time.h>
#include "graph.h"
#include "../../io/include/IBRSocket.h"

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
    //FD_SET(listner_fd, &server_fds);
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
                    if ((nbyte = recv(i, revBuf, sizeof(revBuf), 0)) <= 0) {
                        if (nbyte == 0) {       // client has closed connection
                            printf("selectserver: socket %d hung up\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i);
                        FD_CLR(i, &master);
                        FD_CLR(i, &server_fds);
                        FD_CLR(i, &client_fds);
                        quit(i);
                    } else {        // get some Data from clients or servers
                        if (FD_ISSET(i, &server_fds)) {
                            std::cout << "server connection revBuf: " << revBuf+CMD_SIZE << "\n";
                            handle_server_update(i, revBuf);
                        } else if (FD_ISSET(i, &client_fds)) {
                            std::cout << "client connection revBuf: " << revBuf+CMD_SIZE << "\n";
                            handle_client(i, revBuf);
                        } else {
                            std::cout << "first connection revBuf: " << revBuf+CMD_SIZE << "\n";
                            handle_handshake(i, revBuf);
                        }
                    }
                }
            }
        }
    }
}

void Server::handle_handshake(int &sock, char* buf)
{
    if (*((short*) buf) == IC_CLIENT) {    // client connection
        FD_SET(sock, &client_fds);
        handle_client_handshake(sock, buf);
    } else if (*((short*) buf) == IC_SERVER) {
        FD_SET(sock, &server_fds);
        handle_server_handshake(sock, buf);
    }
}

void Server::add_new_client(char* msg)
{
    s2c_t conn;
    strcpy(conn.conn, msg+CMD_SIZE);
    std::cout << "add client: " << msg+CMD_SIZE << "\n";
    s2c_connections.push_back(conn);
}

void Server::remove_client(char* buf)
{
    for (unsigned int i = 0; i < s2c_connections.size(); i++) {
        if(strcmp(buf+CMD_SIZE, s2c_connections.at(i).conn) == 0) {
            std::cout << "remove client: " << buf+CMD_SIZE << "\n";
            s2c_connections.erase(s2c_connections.begin()+i);
            return;
        }
    }
}

void Server::leave_channel(char* buf)
{
    for (unsigned int i = 0; i < ch2c_connections.size(); i++) {
        if (strcmp(buf+CMD_SIZE, ch2c_connections.at(i).conn) == 0) {
            std::cout << "leave remove: " << buf+CMD_SIZE << "\n";
            ch2c_connections.erase(ch2c_connections.begin()+i);
            break;
        }
    }
}

void Server::join_channel(char* buf)
{
    ch2c_t conn;
    strcpy(conn.conn, buf+CMD_SIZE);
    std::cout << "join add: " << conn.conn << "\n";
    ch2c_connections.push_back(conn);
}

void Server::add_new_server(char* msg)
{
    s2s_t conn;
    char* token = strtok(msg+CMD_SIZE, ";");
    strcpy(conn.conn, token);
    s2s_connections.push_back(conn);

    token = strtok(NULL, ";");
    while(token != NULL) {
        strcpy(conn.conn, token);
        s2s_connections.push_back(conn);
        token = strtok(NULL, ";");
    }
}

void Server::set_channel_topic(char* msg)
{
    ch_topic conn;
    strcpy(conn.conn, msg+CMD_SIZE);

    char* channel = strtok(msg+CMD_SIZE, ":");
    for (unsigned int i = 0; i < ch_topics.size(); i++) {
        if (strncmp(channel, ch_topics.at(i).conn, strlen(channel)) == 0) {
            ch_topics.erase(ch_topics.begin() + i);
            break;
        }
    }
    ch_topics.push_back(conn);
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
        if (recv(server_fd, buf, sizeof (buf), 0) <= 0) {
            //perror("recv");
            //std::cout << "Failed to recieve update Info from server\n";
        } else {
            //handle_recvMsg(server_fd, buf);
            std::cout << "buf: " << buf+CMD_SIZE << ";" << std::endl;
            handle_server_update(server_fd, buf);

            memset(buf, 0, sizeof (buf));    // clear buffer
        }
    }
}

void Server::update_info(int &socket, int type, char* msg)
{
    mutex.lock();
    memset(sendBuf, 0, sizeof(sendBuf));
    if (type >= 0) {
        pack_cmd(type, sendBuf);
        strcpy(sendBuf+CMD_SIZE, msg);
    } else {
        strncpy(sendBuf, msg, BUFFER_SIZE);
    }
    mutex.unlock();

    for (auto &it : servers) {
        if (it.socket != socket) {      // send info to other child servers
            std::cout << "send info to other servers: " << msg << "\n";
            send_data(it.socket, sendBuf);
        }
    }
    // Send info to parent server
    if (server_fd != 0 && server_fd != socket) {
        std::cout << "send info to other servers: " << msg << "\n";
        send_data(server_fd, sendBuf);
    }

    sleep(1);
}

void Server::pack_cmd(unsigned int cmd, char* buf)
{
    int temp = cmd;
    unsigned char* ptr = (unsigned char*) &temp;
    buf[0] = ptr[0];
    buf[1] = ptr[1];
}

void Server::pack_s2s_message(char* msg, s2s_t &conn)
{
    pack_cmd(SC_SERVER_NEW, msg);
    strcat(msg+CMD_SIZE, conn.conn);
    strcat(msg+CMD_SIZE, ";");
}

void Server::pack_s2s_messages(char* msg)
{
    pack_cmd(SC_SERVER_NEW, msg);
    strcat(msg+CMD_SIZE, s2s_connections.at(0).conn);
    for (unsigned int i = 1; i < s2s_connections.size(); i++) {
        strcat(msg+CMD_SIZE, ";");
        strcat(msg+CMD_SIZE, s2s_connections.at(i).conn);
    }
}

void Server::unpack_update_info(int &sock, char* msg)
{
    switch (*((short*) msg)) {
        case SC_CLIENT_NEW:
            add_new_client(msg);
            break;
        case SC_CLIENT_REMOVE:
            remove_client(msg);
            break;
        case SC_SERVER_NEW:
            add_new_server(msg);
            break;
        case SC_CLIENT_LEAVE_CHANNEL:
            std::cout << "leave channel\n";
            leave_channel(msg);
            break;
        case SC_CLIENT_JOIN_CHANNEL:
            join_channel(msg);
            break;
        case SC_CHANNEL_TOPIC:
            set_channel_topic(msg);
            break;
        case SC_MSG:
            std::cout << "send to all\n";
            send_to_all(sock, msg);
            break;
        case SC_PRIVMSG:
            std::cout << "send to one\n";
            send_to_one(sock, msg);
            break;
        default:
            std::cerr << "Unvalid Command from server\n";
    }
}

void Server::handshake_to_server(int &socket)
{
    char msgS[BUFFER_SIZE] = {0};       // sh -> from "server" "handshake"
    pack_cmd(IC_SERVER, msgS);
    strcpy(msgS+CMD_SIZE, this->ID);             // Form "sc[ID]"
    send_data(socket, msgS);

    char msgC[BUFFER_SIZE] = {0};
    if (recv(socket, msgC, sizeof (msgC), 0) <= 0) {
        std::cerr <<"Failed to receive message from Server\n";
    }
    std::cout << "handshake from server: " << msgC+CMD_SIZE << std::endl;
    handle_server_update(socket, msgC);
}

void Server::handle_server_handshake(int &sock, char* buf)
{
    server_t new_serv;
    substring(new_serv.ID, buf, CMD_SIZE, sizeof(new_serv.ID));
    new_serv.socket = sock;
    std::cout << "new_server: " << new_serv.ID << "\n";
    servers.push_back(new_serv);
    s2s_t conn;
    strcpy(conn.conn, this->ID);
    strcat(conn.conn, "-");
    strcat(conn.conn, new_serv.ID);
    s2s_connections.push_back(conn);

    update_info(sock, SC_SERVER_NEW, conn.conn);

    // update all info to new server
    memset(this->sendBuf, 0, sizeof(sendBuf));
    pack_s2s_messages(sendBuf);
    send(sock, sendBuf, sizeof(sendBuf), 0);
}

void Server::handle_server_update(int &sock,  char* buf)
{
    char temp[strlen(buf+CMD_SIZE) + CMD_SIZE] = {0};
    if (*((short*) buf) == SC_CLIENT_NEW) {   // change server ID to this->ID
        s2c_t conn;
        strcpy(conn.conn, this->ID);
        strcat(conn.conn, "-");
        strcpy(temp, buf+CMD_SIZE);
        strtok(temp, "-");
        strcat(conn.conn, strtok(NULL, "-"));

        update_info(sock, SC_CLIENT_NEW, conn.conn);
    } else if (*((short*) buf) == SC_CLIENT_REMOVE) {
        s2c_t conn;
        strcpy(conn.conn, this->ID);
        strcat(conn.conn, "-");
        strcpy(temp, buf+CMD_SIZE);
        strtok(temp, "-");
        strcat(conn.conn, strtok(NULL, "-"));

        update_info(sock, SC_CLIENT_REMOVE, conn.conn);
    } else if (*((short*) buf) != SC_MSG && *((short*) buf) != SC_PRIVMSG) {
        update_info(sock, *((short*) buf), buf+CMD_SIZE);
    }

    unpack_update_info(sock, buf);
}

bool Server::handle_client_handshake(int &sock, char* buf)
{
    if (!is_nick_valid(buf+CMD_SIZE)) {
        handle_confirm(sock, UNVALID_CLIENT_NAME);
        return false;
    }

    client_t new_client;
    new_client.socket = sock;
    strcpy(new_client.nick, buf+CMD_SIZE);
    clients.push_back(new_client);
    std::cout << "new client " << new_client.nick << " has connected\n";
    std::cout << "server has " << clients.size() << " clients\n";

    // update to other server
    s2c_t conn;
    strcpy(conn.conn, this->ID);
    strcat(conn.conn, "-");
    strcat(conn.conn, new_client.nick);

    update_info(sock, SC_CLIENT_NEW, conn.conn);
    handle_confirm(sock, VALID_CLIENT_NAME);
    return true;
}

bool Server::is_nick_valid(char *nick)
{
    // Search all clients under this servers
    for (auto &it : clients) {
        if (strcmp(it.nick, nick) == 0) {
            return false;
        }
    }
    for (auto &ch : channels) {
        for (auto &client : ch.clients) {
            if (strcmp(client.nick, nick) == 0) {
                return false;
            }
        }
    }
    for (auto &it : s2c_connections) {
        if (strstr(it.conn, nick) != NULL) {
            return false;
        }
    }
    return true;
}

bool Server::is_channel_existed(char *channel)
{
    for (auto &conn : ch2c_connections) {
        if (strncmp(channel, conn.conn, strlen(channel)) == 0) {
            return true;
        }
    }
    return false;
}

void Server::handle_client(int &sock, char *buf)
{
    switch (*((short*) buf)) {
        case IC_CLIENT:
            handle_client_handshake(sock, buf);
            break;
        case JOIN:
            join(sock, buf);
            break;
        case LEAVE:
            leave(sock);
            break;
        case NICK:
            change_nick(sock, buf);
            break;
        case LIST:
            list_channels(sock);
            break;
        case GETTOPIC:
            get_topic(sock, buf);
            break;
        case SETTOPIC:
            set_topic(sock, buf);
            break;
        case MSG:
            std::cout << "sent to all\n";
            send_to_all(sock, buf);
            break;
        case PRIVMSG:
            send_to_one(sock, buf);
            break;
        case QUIT:
            std::cout << "client quit\n";
            quit(sock);
            break;
        default:
            std::cerr << "Unvalid Command from client\n";
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
            char temp[strlen(it.conn) + 1] = {0};
            strcpy(temp, it.conn);
            strcpy(server, strtok(temp, "-"));
            std::cout << "get_server: " << server << "\n";
            return true;
        }
    }
    return false;
}

int Server::get_next_hop(char *server)
{
    for (auto &it : servers) {
        if (strcmp(server, it.ID) == 0) {
            return it.socket;
        }
    }

    return server_fd;
}

void Server::send_to_one(int &sock, char *buf)
{
    char name[NICK_SIZE] = {0};
    int size = strlen(buf+CMD_SIZE) + CMD_SIZE;
    char cp_buf[size+1] = {0};
    strncpy(cp_buf, buf+CMD_SIZE, size);
    char *channel = strtok(cp_buf, ":");
    strcpy(name, strtok(NULL, ":"));

    int fd = get_fd(name, channel);
    if (fd != -1) {
        int size = strlen(name) + strlen(channel) + CMD_SIZE + 2;
        if (send(fd, buf+size, strlen(buf+size), 0) <= 0) {   // send message direct to client
            std::cout << "Failed to send to client " << name << "\n";
        } else {
            if (*((short*) buf) != SC_PRIVMSG) {
                handle_confirm(sock, VALID_CLIENT);
            }
        }
    } else {
        char server[INET6_ADDRSTRLEN] = {0};
        if (client_match_channel(name, channel)) {
            if (get_server(name, server)) {
                fd = get_next_hop(server);
                if (fd > 0) {
                    if (*((short*) buf) != SC_PRIVMSG) {
                        handle_confirm(sock, VALID_CLIENT);
                    }
                    pack_cmd(SC_PRIVMSG, buf);
                    send_data(fd, buf);
                } else {
                    std::cerr << "Cannot find next hop\n";
                }
            } else {
                // !!!
                std::cerr << "Cannot find client mit name " << name << "\n";
            }
        } else {
            std::cerr << "Client [" << name << "] do not under channel: " << channel << "\n";
            handle_confirm(sock, UNVALID_CLIENT);
        }
    }
}

void Server::send_to_all(int &sock, char *buf)
{
    // Send to all other servers
    update_info(sock, SC_MSG, buf+CMD_SIZE);

    // Send to all clients who are under same channel
    char channel[CHANNEL_SIZE];
    char temp[strlen(buf+CMD_SIZE)+1] = {0};
    strcpy(temp, buf+CMD_SIZE);
    strcpy(channel, strtok(buf+CMD_SIZE, ":"));
    //char *msg = strtok(NULL, ";");
    for (auto &it : channels) {
        if (strcmp(it.ID, channel) == 0) {
            for (auto &client : it.clients) {
                if (client.socket != sock) {        // Don't send to him self
                    if (send(client.socket, temp+strlen(channel)+1, strlen(temp+strlen(channel)), 0) <= 0) {
                        perror("send");
                        std::cerr << "Failed to send message to client [" << client.nick << "]" << "\n";
                    }
                }
            }
            return;
        }
    }
}

void Server::handle_confirm(int &sock, int confirm)
{
    char buf[3] = {0};
    switch (confirm) {
        case UNVALID_CLIENT_NAME:
            pack_cmd(UNVALID_CLIENT_NAME, buf);
            break;
        case UNVALID_CLIENT:
            pack_cmd(UNVALID_CLIENT, buf);
            break;
        case VALID_CLIENT:
            pack_cmd(VALID_CLIENT, buf);
            break;
        case VALID_CLIENT_NAME:
            pack_cmd(VALID_CLIENT_NAME, buf);
            break;
        default:
            break;
    }
    if (send(sock, buf, sizeof(buf), 0) <= 0) {
        std::cerr << "Failed to handle error to client\n";
    }
}

void Server::join(int &sock, char *buf)
{
    bool is_success = false;
    bool valid_ch = false;
    int index;
    ch2c_t conn;
    char channel[CHANNEL_SIZE] = {0};
    strcpy(channel, buf+CMD_SIZE);
    strcpy(conn.conn, channel);
    strcat(conn.conn, ":");

    // find this client from this->clients vector
    for (unsigned int i = 0; i < this->clients.size(); i++) {
        if (clients.at(i).socket == sock) {
            index = i;
            strcat(conn.conn, clients.at(index).nick);
            is_success = true;
        }
    }
    // add this client to channel->clients vector
    // remove this client from clients vector
    if (is_success) {
        for (auto &it : channels) {
           if (strcmp(it.ID, channel) == 0) {
               it.clients.push_back(clients.at(index));
               std::cout << clients.at(index).nick << "has joined channel: " << channel << "\n";
               clients.erase(clients.begin() + index);
               valid_ch = true;
               break;
           }
        }
        if (!valid_ch) {    // Create new channel under this server
            channel_t ch;
            strcpy(ch.ID, channel);
            if (!is_channel_existed(channel)) {
                strcpy(ch.creator, clients.at(index).nick);
                std::cout << clients.at(index).nick << " create a new channel named: " << channel << "\n";
            }
            std::cout << clients.at(index).nick << "has joined channel: " << channel << "\n";
            ch.clients.push_back(clients.at(index));
            clients.erase(clients.begin() + index);
            channels.push_back(ch);
        }
        std::cout << "conn.conn: " << conn.conn << "\n";
        // Send update info to other servers
        update_info(sock, SC_CLIENT_JOIN_CHANNEL, conn.conn);
    }
}

void Server::leave(int &sock)
{
    ch2c_t conn;
    for (auto &channel : channels) {
        for (unsigned int i = 0; i < channel.clients.size(); i++) {
            if (sock == channel.clients.at(i).socket) {
                strcpy(conn.conn, channel.ID);
                strcat(conn.conn, ":");
                strcat(conn.conn, channel.clients.at(i).nick);

                clients.push_back(channel.clients.at(i));
                channel.clients.erase(channel.clients.begin() + i);
                
                // update to othre servers
                update_info(sock, SC_CLIENT_LEAVE_CHANNEL, conn.conn);
                return;
            }
        }
    }
}

void Server::change_nick(int &sock, char* buf)
{
    bool isFound = false;
    char name[NICK_SIZE] = {0};
    strcpy(name, buf+CMD_SIZE);
    if (!is_nick_valid(name)) {
        handle_confirm(sock, UNVALID_CLIENT_NAME);
        return;
    } else {
        handle_confirm(sock, VALID_CLIENT_NAME);
    }
    s2c_t conn;
    strcpy(conn.conn, this->ID);
    strcat(conn.conn, "-");
    strcat(conn.conn, name);

    update_info(sock, SC_CLIENT_NEW, conn.conn);
    for (auto &client : clients) {
        if (sock == client.socket) {
            conn.conn[0] = 0;
            strcpy(conn.conn, this->ID);
            strcat(conn.conn, "-");
            strcat(conn.conn, client.nick);

            client.nick[0] = 0;
            strcpy(client.nick, name);
            isFound = true;
        }
    }
    if (!isFound) {
        for (auto &channel : channels) {
            for (unsigned int i = 0; i < channel.clients.size(); i++) {
                if (sock == channel.clients.at(i).socket) {
                    ch2c_t conn_ch2c;
                    strcpy(conn_ch2c.conn, channel.ID);
                    strcat(conn_ch2c.conn, ":");
                    strcat(conn_ch2c.conn, channel.clients.at(i).nick);
                    update_info(sock, SC_CLIENT_LEAVE_CHANNEL, conn_ch2c.conn);

                    conn_ch2c.conn[0] = 0;
                    strcpy(conn_ch2c.conn, channel.ID);
                    strcat(conn_ch2c.conn, ":");
                    strcat(conn_ch2c.conn, name);
                    update_info(sock, SC_CLIENT_JOIN_CHANNEL, conn_ch2c.conn);

                    conn.conn[0] = 0;
                    strcpy(conn.conn, this->ID);
                    strcat(conn.conn, "-");
                    strcat(conn.conn, channel.clients.at(i).nick);

                    // change channel creator, if this client is creator of this channel 
                    if (strcmp(channel.creator, channel.clients.at(i).nick) == 0) {
                        channel.creator[0] = 0;
                        strcpy(channel.creator, name);
                    }

                    // change nick name
                    channel.clients.at(i).nick[0] = 0;
                    strcpy(channel.clients.at(i).nick, name);
                }
            }   
        }
    }
    
    // remove old client in other server
    update_info(sock, SC_CLIENT_REMOVE, conn.conn);
}

void Server::list_channels(int &sock)
{
    char ch_list[BUFFER_SIZE] = {0};
    for (auto &channel : channels) {
        strcat(ch_list, channel.ID);
        strcat(ch_list, "\n");
    }
    for (auto &ch2c : ch2c_connections) {
        char temp[CHANNEL_SIZE+1] = {0};
        strcpy(temp, strtok(ch2c.conn, ":"));
        if (strstr(temp, ch_list) == NULL) {
            strcat(ch_list, temp);
            strcat(ch_list, "\n");
        }
    }
    send_data(sock, ch_list);
}

void Server::get_topic(int &sock, char *topic)
{
    for (auto &ch_topic : ch_topics) {
        if (strncmp(topic+CMD_SIZE, ch_topic.conn, strlen(topic+CMD_SIZE)) == 0) {
            send_data(sock, ch_topic.conn);
            return;
        }
    }

    for (auto &channel : channels) {
        if (strcmp(topic+CMD_SIZE, channel.ID) == 0) {
            send_data(sock, channel.topic);
            return;
        }
    }
}

void Server::set_topic(int &sock, char *topic)
{
    for (auto &channel : channels) {
        for (auto &client : channel.clients) {
            if (client.socket == sock) {
                if (strcmp(channel.creator, client.nick) == 0) {
                    channel.topic[0] = 0;
                    strcpy(channel.topic, topic+CMD_SIZE);
                    ch_topic conn;
                    strcpy(conn.conn, channel.ID);
                    strcat(conn.conn, ":");
                    strcat(conn.conn, channel.topic);
                    update_info(sock, SC_CHANNEL_TOPIC, conn.conn);
                    return;
                }
            }
        }
    }
}

void Server::quit(int &sock)
{
    // Not in a channel
    for (unsigned int i = 0; i < clients.size(); i++) {
        if (clients.at(i).socket == sock) {
            s2c_t conn;
            strcpy(conn.conn, this->ID);
            strcat(conn.conn, "-");
            strcat(conn.conn, clients.at(i).nick);
            update_info(sock, SC_CLIENT_REMOVE, conn.conn);
            clients.erase(clients.begin()+i);
            return;
        }
    }
    // In a channel
    for (auto &channel : channels) {
        for (unsigned int i = 0; i < channel.clients.size(); i++) {
            if (channel.clients.at(i).socket == sock) {
                ch2c_t ch_conn;
                strcpy(ch_conn.conn, channel.ID);
                strcat(ch_conn.conn, ":");
                strcat(ch_conn.conn, channel.clients.at(i).nick);
                update_info(sock, SC_CLIENT_LEAVE_CHANNEL, ch_conn.conn);

                s2c_t conn;
                strcpy(conn.conn, this->ID);
                strcat(conn.conn, "-");
                strcat(conn.conn, channel.clients.at(i).nick);
                update_info(sock, SC_CLIENT_REMOVE, conn.conn);

                channel.clients.erase(channel.clients.begin()+i);
                return;
            }
        }
    }
}

void Server::substring(char* dest, char* src, const unsigned int &start, const unsigned int &length)
{
    for (unsigned int i = start; i < start + length; i++) {
        dest[i-start] = src[i];
    }
}

void Server::send_data(int &sock, char *buf)
{
    if (send(sock, buf, strlen(buf+CMD_SIZE)+CMD_SIZE, 0) <= 0) {
        std::cerr << "Failed to send Data\n";
    }
}

void* Server::get_in_addr(struct sockaddr* sa)
{
    if (sa->sa_family == AF_INET) {
            return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
