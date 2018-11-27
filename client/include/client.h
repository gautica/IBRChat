#define CLIENT_H

class Client

#define QUIT_CMD "/quit"
#define LEAVE_CMD "/leave"
#define HELP_CMD "/help"
#define JOIN_CMD "/join "
#define NICK_CMD "/nick "
#define LIST_CMD "/list"
#define GTOPIC_CMD "/gettopic "
#define STOPIC_CMD "/settopic "
#define PRIVMSG_CMD "/privmsg "

{
public:
    /**
     * @brief starts the client and asks the user to select a user name
     */
    void start_client();
    /**
     * @brief deals with the input the user gives the client
     */
    void handle_input();
    /**
     * @brief connects to a server with the given IP and port
     * @param IP: IP addr from the server the client should connect to
     * @param port: port from the server the client should connect to
     */
    void connect_to_server(const char* IP, const char* port);
    /**
     * @brief waits for messages from the server
     */
    void recv_messages();

private:
    /**
     * @brief handles input if the input was a command (begins with '/')
     */
    void handle_command(char input[]);
    /**
     * @brief sends a message to the socket from the server the client is conntected to
     * @param the message the client should send
     */
    void send_message(char message[]);
    /**
     * @brief lists all commands
     */
    void list_commands();
    /**
     * @brief sends command to a server
     */
    void send_command(short cmd);

};

