#define CLIENT_H

class Client
{
public:
    void start_client();
    /**
     * @brief deals with the input the user gives the client
     */
    void handle_input();

private:
    /**
     * @brief connects to a server with the given IP and port
     */
    void connect_to_server();
    /**
     * @brief sends a message to the socket from the server the client is conntected to
     */
    void send_message();
    /**
     * @brief recv_message
     */
    void recv_message();
    /**
     * @brief lists all commands
     */
    void list_commands();

};
