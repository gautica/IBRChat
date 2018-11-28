#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include "client.h"
#include "../../io/include/IBRSocket.h"

#define MAX_INPUT_LENGTH 256
#define CMD_SIZE 2

const char divider[] = ":";
int soc;
char username[9]; //max. length is 9
char channelname[12]; //max. length is 12

void Client::start_client() {
    while(true) {
        printf("please select a username: ");
        scanf("%s", username);
        if (strlen(username) <= 9 && strlen(username) > 0) {
            char handshake[11];
            pack_cmd(IC_CLIENT, handshake);
            strcat(handshake + 2, username);
            //send ch[username], so server can check if avaiable
            send_message(handshake);
            //recv if name was available
            char confirm[MAX_INPUT_LENGTH];
            if (recv(soc, confirm, MAX_INPUT_LENGTH-1, 0)!= -1) {
                printf("CONFIRM: %s\n", confirm);
                if (confirm[0] == 'H') { //Hello, I'm the server with IP: [IP]:[PORT]
                    break;
                } else {
                    printf("[SERVER]: %s\n", confirm);
                }
            } else {
                printf("[WARNING] could not handshake with the server\n");
            }
        } else {
            printf("[WARNING] username too long/short, please try again [1-9] letters\n");
        }
    }
    printf("your username was set to [%s]\n", username);
}

void Client::handle_input() {
    while (true) {
        printf("[%s][@%s]: ", username, channelname);
        char input[MAX_INPUT_LENGTH];
        fgets(input, MAX_INPUT_LENGTH, stdin);
        //checks if command was used
        if (input[0] == '/') {
            handle_command(input);
        } else {
            //if no command was used, sends message to the channel
            //send MSG[channel]:[message] to server
            char message[MAX_INPUT_LENGTH + strlen(channelname) + strlen(divider) + CMD_SIZE] = {0};
            pack_cmd(MSG, message);
            strcat(message + CMD_SIZE, channelname);
            strcat(message + CMD_SIZE, divider);
            strcat(message + CMD_SIZE, input);
            //printf("MSG: %d:%d:%s\n", message[0], message[1], message + 2);
            send_message(message);
        }
    }
}

void Client::handle_command(char input[]) {
    //send cb[cmd] to server if needed
    char temp[MAX_INPUT_LENGTH + CMD_SIZE] = {0};
    if (strncmp(QUIT_CMD, input, strlen(QUIT_CMD)) == 0) {
        printf("see you soon\n");
        send_command(QUIT);
        exit(EXIT_SUCCESS);
    } else if (strncmp(LEAVE_CMD, input, strlen(LEAVE_CMD)) == 0) {
        send_command(LEAVE);
        memset(channelname, 0, sizeof(channelname));
    } else if (strncmp(HELP_CMD, input, strlen(HELP_CMD)) == 0) {
        list_commands();
    } else if (strncmp(JOIN_CMD, input, strlen(JOIN_CMD)) == 0) {
        pack_cmd(JOIN, temp);
        strcpy(temp + CMD_SIZE, input + strlen(JOIN_CMD));
        send_message(temp);
        strcpy(channelname, temp+CMD_SIZE);
    } else if (strncmp(NICK_CMD, input, strlen(NICK_CMD)) == 0) {
        send_command(NICK);
    } else if (strncmp(LIST_CMD, input, strlen(LIST_CMD)) == 0) {
        send_command(LIST);
    } else if (strncmp(GTOPIC_CMD, input, strlen(GTOPIC_CMD)) == 0) {
        send_command(GETTOPIC);
    } else if (strncmp(STOPIC_CMD, input, strlen(STOPIC_CMD)) == 0) {
        send_command(SETTOPIC);
    } else if (strncmp(PRIVMSG_CMD, input, strlen(PRIVMSG_CMD)) == 0) {
        // send privmsg, target:channel:name
        send_command(PRIVMSG);
    }
}

void Client::connect_to_server(const char* IP, const char* port) {
    addrinfo hints;
    addrinfo* res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(IP, port, &hints, &res);
    soc = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (connect(soc, res->ai_addr, res->ai_addrlen) == -1) {
        perror("[WARNING] could not connect to the server");
        exit(EXIT_FAILURE);
    }
}

void Client::send_command(short cmd) {
    if (send(soc, &cmd, 2, 0) == -1) {
        perror("[WARNING] an error occured while sending the command, please try again");
    }
}

void Client::send_message(char* message) {
    if (send(soc, message, strlen(message+2) + CMD_SIZE, 0) == -1) {
        perror("[WARNING] an error occured while sending the message, please try again");
    }
}

void Client::recv_messages() {
    while (true) {
        char recvMsg[MAX_INPUT_LENGTH];
        if (recv(soc, recvMsg, MAX_INPUT_LENGTH-1, 0) == -1) {
            perror("[WARNING] an error occured while reciving a message");
        }
        printf("%s\n", recvMsg);
    }
}

void Client::list_commands() {
    printf ("-----------------------------------------------------------------------\n");
    printf("possible commands:\n");
    printf("/join <channel> - joins the selcted channel\n");
    printf("/leave - leaves the current channel\n");
    printf("/nick <name> - sets the current nickname of the user to <name>\n");
    printf("/list - lists all known channels and topic\n");
    printf("/gettopic <channel> - prints the topic of the <channel>\n");
    printf("/settopic <channel> - sets the topic of the <channel>\n");
    printf("/privmsg <name> <message> - sends private <message> to the user <name>\n");
    printf("/quit - stops the IBRC and leaves the current channel\n");
    printf ("-----------------------------------------------------------------------\n");
}

void Client::pack_cmd(unsigned int cmd, char* buf) {
    int temp = cmd;
    unsigned char *ptr = (unsigned char*) &temp;
    buf[0] = ptr[0];
    buf[1] = ptr[1];
}

