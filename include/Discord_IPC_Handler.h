#ifndef DISCORD_IPC_HANDLER_H
#define DISCORD_IPC_HANDLER_H

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sstream>
#include <chrono>
#include <vector>
#include <sys/socket.h>
#include <sys/un.h>

class Discord_IPC_handler
{
    public:
    Discord_IPC_handler() : fd(-1), connected(false) {   }
    ~Discord_IPC_handler()
    {
        discord_disconnect();
    }
    bool connect_to_discord(const std::string &app_ID_param);
    bool set_activity(const std::string &details, const std::string &state, const std::string &largeImageKey = "", const std::string &largeImageText = "", bool record_time=true, int64_t discord_timestamp=0);
    bool clear_activity();
    void discord_disconnect();
    bool is_connected();

private:
    int fd;
    bool connected;
    std::string application_ID;

    enum class OpCode : uint32_t
    {
        HANDSHAKE = 0, //the message used for our initial connection
        FRAME = 1, //used for the JSON commands we send to Discord
        CLOSE = 2, //used for when we are done and want to close the connection
        PING = 3, //ping to discord
        PONG = 4 //pong response to a ping
    };

    std::vector<uint8_t> pack_message(OpCode opcode, const std::string &data); //function to pack our data into Discord IPC format
    bool connect_to_socket();
    bool send_handshake();
    bool read_response();
};

#endif
