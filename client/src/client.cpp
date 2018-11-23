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
        //TODO: server ch[nickname] senden, also Handshake
        printf("please select a username: ");
        scanf("%s", username);
        if (strlen(username) <= 9 && strlen(username) > 0) {
            //send
            //recv
            break;
        } else {
            printf("[WARNING] username too long/short, please try again [1-9] letters\n");
        }
    }
    printf("your username was set to [%s]\n", username);
}

void Client::handle_input() {
    while (true) {
        char input[MAX_INPUT_LENGTH];
        scanf("%s", input);
        //checks if command was used
        if (input[0] == '/') {
            handle_command(input);
        } else {
            //if no command was used, sends message to the channel
            // send_message() ....
        }
    }
}

void Client::handle_command(char input[]) {
    if (strcmp("/quit", input) == 0) {
        printf("see you soon\n");
        exit(EXIT_SUCCESS);
    } else if (strcmp("/leave", input) == 0) {
        //TODO: leave current channel and send this action to the server
    } else if (strcmp("/list", input) == 0) {
        //TODO: send message to server to get all channels and recv answer and print answer
    } else if (strcmp("/help", input) == 0) {
        list_commands();
    }
    //sende cb[COMMAND]
    //nutze strtok
    //join <name>
    //nick <name>
    //get topic <channel>
    //set topic <channel>
    //privmsg <name> <message>
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
        perror("[WARNING] an error occured while sending the message, please try again\n");
    }
}

//auf neuen Thread
void Client::recv_message() {

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






