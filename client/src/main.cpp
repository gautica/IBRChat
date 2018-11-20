#include "../include/client.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    Client client;
    client.start_client();
    client.connect_to_server(argv[1], argv[2]);
    client.handle_input();
}
