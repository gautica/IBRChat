#include "server.h"



int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: <server> <IP> [port_to_client] [port_from_server]\n");
        exit(EXIT_FAILURE);
    }
    /**
     * argv[1]: IP (required)
     * argv[2]: port to client (optional)
     * argv[3]: port from server (optional)
     */
    Server server(argv[1], argv[2], argv[3]);

    server.start();

    return 0;
}
