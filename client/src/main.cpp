#include "client.h"
#include <thread>
#include <stdio.h>

int main(int argc, char *argv[]) {
    Client client;
    if (argv[1] == NULL || argv[2] == NULL) {
        printf("USE ./client [IP] [PORT]\n");
        exit(EXIT_FAILURE);
    }
    client.connect_to_server(argv[1], argv[2]);
    client.start_client();

    //make a thread for user to give input and a thread to recv input from server
    client.handle_input();
    std::thread (&Client::handle_input, client);
    std::thread (&Client::recv_messages, client);
}
