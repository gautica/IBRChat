#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include "../include/client.h"

#define default_PORT "5000"
#define MAX_INPUT_LENGTH 256

int soc;
char username[9];
char channelName[13] = "-";

void Client::start_client() {
    printf("please select a username:\n");
    scanf("%s", username);
    printf("your username was set to [%s]\n", username);
    printf("choose action: \n");
}

void Client::handle_input() {
    while (true) {
        // scans input from the user:
        char input[MAX_INPUT_LENGTH];
        scanf("%s", input);
        //checks if command was used
        if (input[0] == '/') {

        } else {
            //if no command was used, sends message to the channel
            printf("%s: %s\n", channelName, input);
        }
    }
}

void connect_to_server() {
    addrinfo hints;
    addrinfo* res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo("localhost", default_PORT, &hints, &res);
    soc = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    connect(soc, res->ai_addr, res->ai_addrlen);
}

void send_message(char* message) {
    if (send(soc, message, sizeof(message), 0) == -1) {
        perror("an error occured while sending the message");
    }
    printf("%s: ", username);
    printf("%s\n", message);
}

void recv_message() {

}

void list_commands() {
    printf("possible commands:\n");
    printf("/join <channel> - joins the selcted channel\n");
    printf("/leave - leaves the current channel\n");
    printf("/nick <name> - sets the current nickname of the user to <name>\n");
    printf("/list - lists all known channels and topic\n");
    printf("/gettopic <channel> - prints the topic of the <channel>\n");
    printf("/settopic <channel> - sets the topic of the <channel>\n");
    printf("/privmsg <name> <message> - sends private <message> to the user <name>\n");
    printf("/quit - stops the IBRC and leaves the current channel\n");
}






