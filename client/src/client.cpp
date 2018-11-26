#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include "client.h"

#define MAX_INPUT_LENGTH 256

int soc;
char username[9]; //max. length is 9

void Client::start_client() {
    while(true) {
        printf("please select a username: ");
        scanf("%s", username);
        if (strlen(username) <= 9 && strlen(username) > 0) {

            //-------------------------------------------------
            break; //delete this when server connection works
            //-------------------------------------------------

            //send ch[username], so server can check if avaiable
            send_message(username);
            //recv if name was available
            char confirm[MAX_INPUT_LENGTH];
            if (recv(soc, confirm, MAX_INPUT_LENGTH-1, 0)!= -1) {
                if (confirm[0] == '1') {
                    break;
                } else {
                    printf("[WARNING] this username is already taken, please try again\n");
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
        //printf("%s: ", username);
        char input[MAX_INPUT_LENGTH];
        fgets(input, MAX_INPUT_LENGTH, stdin);
        //checks if command was used
        if (input[0] == '/') {
            handle_command(input);
        } else {
            //if no command was used, sends message to the channel
            //send_message(); //ccm
        }
    }
}

void Client::handle_command(char input[]) {
    //sende cb[COMMAND]
    if (strncmp("/quit", input, strlen("/quit")) == 0) {
        printf("see you soon\n");
        exit(EXIT_SUCCESS);
    } else if (strncmp("/leave", input, strlen("/leave")) == 0) {
        //send message
    } else if (strncmp("/list", input, strlen("/list")) == 0) {
        //send message
    } else if (strncmp("/help", input, strlen("/help")) == 0) {
        list_commands();
    } else if (strncmp("/nick ", input, strlen("/nick ")) == 0) {
        //username = ..........
    } else if (strncmp("/join ", input, strlen("/join ")) == 0) {
        //send msg
    } else if (strncmp("/gettopic ", input, strlen("/gettopic ")) == 0) {
        //send msg
    } else if (strncmp("/settopic ", input, strlen("/settopic ")) == 0) {
        //send msg
    } else if (strncmp("/privmsg ", input, strlen("/privmsg ")) == 0) {
        //send msg
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
        //exit(EXIT_FAILURE);
    }
}

void Client::send_message(char message[]) {
    if (send(soc, message, strlen(message), 0) == -1) {
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

