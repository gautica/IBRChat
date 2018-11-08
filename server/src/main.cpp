#include "server.h"



int main(int argc, char *argv[])
{
    Server server(argv[1], argv[2], argv[3], argv[4]);

    server.start();
    return 0;
}
