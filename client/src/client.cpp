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
char channelname[12] = {0}; //max. length is 12

void Client::start_client() {
    while(true) {
        printf("please select a username: ");
        char input[MAX_INPUT_LENGTH];
        fgets(input, MAX_INPUT_LENGTH, stdin);
        if(strlen(input) > 10) {
            printf("[WARNING] username too long/short, please try again [1-9] letters\n");
            continue;
        }
        input[strcspn(input, "\n")] = 0; //delete \n from fgets
        int accepted = 1;
        for (int i = 0; i < strlen(input); i++) {
            if (!(input[i] >= 'a' && input[i] <= 'z' || input[i] >= 'A' && input[i] <= 'Z' || input[i] >= '0' && input[i] <= '9')) {
                accepted = 0;
                break;
            }
        }
        if (accepted == 0) {
            printf("[WARNING] wrong letter, use A-Z or 0-9\n");
            continue;
        }
        strncpy(username, input, 9);
        char handshake[12] = {0};
        pack_cmd(IC_CLIENT, handshake);
        strcpy(handshake + 2, username);
        //send ch[username], so server can check if avaiable
        send_message(handshake);
        //recv if name was available
        char confirm[MAX_INPUT_LENGTH];
        if (recv(soc, confirm, MAX_INPUT_LENGTH, 0)!= -1) {
            if (*((short*)confirm) == VALID_CLIENT_NAME) {
                break;
            } else {
                printf("this name is already in use - ");
            }
        } else {
            printf("[WARNING] could not handshake with the server\n");
        }
    }
    printf("your username was set to [%s]\n", username);
}

void Client::handle_input() {
    while (true) {
        printf("[%s][@%s]: ", username, channelname);
        char input[MAX_INPUT_LENGTH];
        if (fgets(input, MAX_INPUT_LENGTH, stdin) != NULL) {
            //printf("INPUT: %s", input);
            input[strcspn(input, "\n")] = 0;
            //checks if command was used
            if (input[0] == '/') {
                handle_command(input);
            } else if (strlen(input) > 0) {
                //if no command was used, sends message to the channel
                //send MSG[channel]:[message] to server
                char message[MAX_INPUT_LENGTH + strlen(channelname) + strlen(divider) + CMD_SIZE] = {0};
                pack_cmd(MSG, message);
                strcat(message + CMD_SIZE, channelname);
                strcat(message + CMD_SIZE, divider);
                strcat(message + CMD_SIZE, input);
                //printf("MSG: %d:%d:%s\n", message[0], message[1], message + CMD_SIZE);
                send_message(message);
            }
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
        if (strlen(channelname) < 1) {
            printf("[ERROR] not member in a channel\n");
            return;
        }
        send_command(LEAVE);
        memset(channelname, 0, sizeof(channelname));
    } else if (strncmp(HELP_CMD, input, strlen(HELP_CMD)) == 0) {
        list_commands();
    } else if (strncmp(JOIN_CMD, input, strlen(JOIN_CMD)) == 0) {
        //check if user is not in a channel
        if(strlen(channelname) > 0) {
            return;
        }
        pack_cmd(JOIN, temp);
        strcpy(temp + CMD_SIZE, input + strlen(JOIN_CMD));
        send_message(temp);
        //printf("temp: %s\n", temp+CMD_SIZE);
        strcpy(channelname, temp+CMD_SIZE);
    } else if (strncmp(NICK_CMD, input, strlen(NICK_CMD)) == 0) {
        //check if new username got the right size
        if(strlen(input) - strlen(NICK_CMD) >= 9) {
            printf("[WARNING] username too long/short, please try again [1-9] letters\n");
            return;
        }
        //send [NICK][username] to server
        for (int i = strlen(NICK_CMD); i < strlen(input); i++) {
            if (!(input[i] >= 'a' && input[i] <= 'z' || input[i] >= 'A' && input[i] <= 'Z' || input[i] >= '0' && input[i] <= '9')) {
                printf("[WARNING] wrong letter, use A-Z or 0-9\n");
                return;
            }
        }
        pack_cmd(NICK, temp);
        strcpy(temp + CMD_SIZE, input + strlen(NICK_CMD));
        send_message(temp);
        //recv 0 when name was accepted, 1 if not
        char buf[2] = {0};
        if (recv(soc, buf, sizeof(buf), 0) != -1) {
            if (*((short*)buf) == VALID_CLIENT_NAME) {
                strncpy(username, temp+CMD_SIZE, sizeof(username));
            } else {
                printf("username already in use\n");
                return;
            }
        } else {
            perror("Could not change username, please try again");
        }
        //printf("[%d%d%s]", temp[0], temp[1], temp + CMD_SIZE);
    } else if (strncmp(LIST_CMD, input, strlen(LIST_CMD)) == 0) {
        send_command(LIST);
        printf("\n known channels:\n");
    } else if (strncmp(GTOPIC_CMD, input, strlen(GTOPIC_CMD)) == 0) {
        //send gettopic channel
        pack_cmd(GETTOPIC, temp);
        strcpy(temp + CMD_SIZE, input + strlen(GTOPIC_CMD));
        send_message(temp);
    } else if (strncmp(STOPIC_CMD, input, strlen(STOPIC_CMD)) == 0) {
        //send settopic channel:newTopic
        pack_cmd(SETTOPIC, temp);
        for(int i = strlen(STOPIC_CMD); i < strlen(input); i++) {
            if(input[i] == ' ') {
                input[i] = ':';
                break;
            }
        }
        strcpy(temp + CMD_SIZE, input + strlen(STOPIC_CMD));
    } else if (strncmp(PRIVMSG_CMD, input, strlen(PRIVMSG_CMD)) == 0) {
        //send privmsg channel:user:message
        pack_cmd(PRIVMSG, temp);
        strcpy(temp + CMD_SIZE, channelname);
        for(int i = strlen(PRIVMSG_CMD); i < strlen(input); i++) {
            if(input[i] == ' ') {
                input[i] = ':';
                break;
            }
        }
        strcpy(temp + CMD_SIZE + strlen(channelname), divider);
        strcpy(temp + CMD_SIZE + strlen(channelname) + strlen(divider), input + strlen(PRIVMSG_CMD));
        send_message(temp);
        char buf[2] = {0};
        if (recv(soc, buf, sizeof(buf), 0) != -1) {
            if (*((short*)buf) == VALID_CLIENT) {
                printf("message send\n");
            } else if (*((short*)buf) == UNVALID_CLIENT) {
                printf("client not found in the current channel, message could not been send\n");
            } else {
                printf("unknown error, could not send message\n");
            }
        }
        //printf("PRIVMSG: %d%d%s\n", temp[0], temp[1], temp+2);
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
    if (send(soc, &cmd, CMD_SIZE, 0) == -1) {
        perror("[WARNING] an error occured while sending the command, please try again");
    }
}

void Client::send_message(char* message) {
    if (send(soc, message, strlen(message+CMD_SIZE) + CMD_SIZE, 0) == -1) {
        perror("[WARNING] an error occured while sending the message, please try again");
    }
}

void Client::recv_messages() {
    while (true) {
        char recvMsg[MAX_INPUT_LENGTH] = {0};
        if (recv(soc, recvMsg, MAX_INPUT_LENGTH-1, 0) != -1) {
            //perror("[WARNING] an error occured while reciving a message");
            if (strlen(recvMsg) > 0) {
                printf("%s\n", recvMsg);
            }
        }       
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
    printf("/settopic <channel> <topic> - sets the topic of the <channel>\n");
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
