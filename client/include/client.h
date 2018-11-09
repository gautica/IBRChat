#define CLIENT_H

class Client
{
public:
    void start_client();

private:
    void handle_input();
    void connect_to_server();
    void send_message();
    void recv_message();
    void list_commands();

};
