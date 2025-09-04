#ifndef DISCORD_IPC_HANDLER_H
#define DISCORD_IPC_HANDLER_H

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <cstdint>
#include <vector>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <thread>
#include <chrono>

/*TODO (In priority order):

~Delete inappropriate comments (like this todo list ;) ) before we send this to Github
~finish the project and release to Github lol
~test this on BSD (unlikely to ever work as I dont think Discord has a native BSD port (can still run linux version with a compatability layer doe)) and MAC (altho since i have no MAC idk how ill do this :/ (maybe you can test on old x86 macs thru vm, but obv im not buying a whole ass Apple silicon MAC))
*/

class Discord_IPC_handler
{
    public:
    Discord_IPC_handler() : fd(-1), connected(false) {   }
    ~Discord_IPC_handler()
    {
        discord_disconnect();
    }
    bool discord_connect(const std::string &app_ID_param);
    bool set_activity(const std::string &details, const std::string &state, const std::string &largeImageKey = "", const std::string &largeImageText = "", bool record_time=true, int64_t discord_timestamp=0);
    bool clear_activity(); //to clear our activity shown on discord
    void discord_disconnect();
    bool is_connected();

private:
    int fd; //the file descriptor representing our socket
    bool connected; //check if we are connected to discord through IPC
    std::string application_ID; //The application ID for Code::Cord

    enum class OpCode : uint32_t
    {
        HANDSHAKE = 0, //the message used for our initial connection
        FRAME = 1, //used for the JSON commands we send to Discord
        CLOSE = 2, //used for when we are done and want to close the connection
        PING = 3, //ping to discord
        PONG = 4 //pong response to a ping
    };

    std::vector<uint8_t> pack_message(OpCode opcode, const std::string &data); //function to pack our data into Discord IPC format
    bool connect_to_socket(); //connect to the Discord IPC socket
    bool send_handshake(); //send handshake message
    bool read_response(); //read response from Discord
};


#endif // DISCORD_IPC_HANDLER_H
