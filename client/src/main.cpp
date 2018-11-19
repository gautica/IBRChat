#include "../include/client.h"

int main(int argc, char *argv[]) {
    Client client;
    client.start_client();
    client.handle_input();
}
