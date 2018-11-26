#include "client.h"
#include <thread>

int main(int argc, char *argv[]) {
    Client client;
    client.connect_to_server(argv[1], argv[2]);
    client.start_client();

    //make a thread for user to give input and a thread to recv input from server
    client.handle_input();
    //std::thread(client.handle_input());
    //std::thread(client.recv_messages());
}
