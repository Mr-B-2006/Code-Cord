#include "Discord_IPC_Handler.h"

bool Discord_IPC_handler::connect_to_discord(const std::string &app_ID_param)
{
    if (connected)
    {
        return true;
    }

    application_ID = app_ID_param;

    if (connect_to_socket()) //connection to discord IPC pipe is successful
    {
        if (send_handshake()) //we can successfully send Discord a handshake
        {
            if (read_response()) //we can successfully receive Discord's handshake
            {
                connected = true;
                std::cout << "Discord IPC handshake successful!" << std::endl;
                return true;
            }
        }
        close(fd); //close our connection
        fd = -1; //error state
    }
    std::cout << "Failed to connect to Discord IPC" << std::endl;
    discord_disconnect();
    return false;
}

bool Discord_IPC_handler::set_activity(const std::string &state, const std::string &details, const std::string &largeImageKey, const std::string &largeImageText, bool record_time, int64_t discord_timestamp)
{ //this function needs to be edited to handle blank parameters, and for us to have a timestamp parameter so we can start recording time on file switch, if the user wants it that way ;)
    if (!connected)
    {
        std::cout << "Not connected to Discord" << std::endl;
        return false;
    }

    int64_t nonce_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(); //gives us the current time in milliseconds, we use this as the nonce we send to Discord (as using seconds can cause duplicate nonces when switching tabs, causing packets to be received out of order)
    //build JSON payload
    std::ostringstream json;
    json << R"({
        "cmd": "SET_ACTIVITY",
        "args":
        {
            "pid": )" << getpid() << R"(,
            "activity":
            {
                "state": ")" << state << R"(",
                "details": ")" << details << R"(",
                "timestamps":
                    {
                    "start": )" << discord_timestamp << R"(
                    })";

    if (!largeImageKey.empty()) //TODO: this but for details, state and timestamp, for user config
    {
        json << R"(,
                "assets":
                {
                    "large_image": ")" << largeImageKey << R"(")";
            if (!largeImageText.empty())
            {
                json << R"(,
                    "large_text": ")" << largeImageText << R"(")";
            }
            json<< "}";
    }

    json << R"(
            }
        },
    "nonce": ")" << nonce_timestamp << R"("
       })";

    std::string payload = json.str();
    auto packet = pack_message(OpCode::FRAME, payload); //packing our JSON command data to change our activity to the discord IPC socket

    ssize_t sent = send(fd, packet.data(), packet.size(), 0);
    if (sent != static_cast<ssize_t>(packet.size())) //if packet didnt send (Didcord likely isnt running)
    {
        std::cout << "Failed to send activity" << std::endl;
        discord_disconnect();
        return false;
    }

    read_response(); //sends back Discords response to our payload (mainly good for error checking)
    return true;
}

bool Discord_IPC_handler::clear_activity()
{
    if (!connected)
    {
        return false;
    }

    std::string payload
    = R"({
            "cmd": "SET_ACTIVITY",
            "args":
            {
                "pid": )" + std::to_string(getpid()) + R"(,
                "activity": null
            }
        })";

    auto packet = pack_message(OpCode::FRAME, payload); //packing our JSON command data to clear our activity...
    ssize_t sent = send(fd, packet.data(), packet.size(), 0); //...so we can send it to the discord IPC socket

    if (sent == static_cast<ssize_t>(packet.size()))
    {
        read_response();
        return true;
    }
    return false;
}

void Discord_IPC_handler::discord_disconnect()
{
    if (connected && fd != -1) //check that the connection isnt already closed
    {
        clear_activity();
        close(fd);
        fd = -1;
        connected = false;
        std::cout << "Disconnected from Discord" << std::endl;
    }
}

bool Discord_IPC_handler::is_connected()
{
    return connected;
}

std::vector<uint8_t> Discord_IPC_handler::pack_message(OpCode opcode, const std::string &data)
{
    std::vector<uint8_t> packet; //the actual data packet we send to Discord

    uint32_t op = static_cast<uint32_t>(opcode); //converts our opcode into 4 bytes
    packet.push_back(op & 0xFF); //bitwise AND done with 11111111, so we only keep the first byte of "op"...
    packet.push_back((op >> 8) & 0xFF); //...then we do this again but shifting right by 8 bits (1 byte), to get our second byte...
    packet.push_back((op >> 16) & 0xFF); //..and so on until byte 4
    packet.push_back((op >> 24) & 0xFF);

    uint32_t length = data.size(); //converts our data length into 4 bytes, like how we did with our opcode
    packet.push_back(length & 0xFF);
    packet.push_back((length >> 8) & 0xFF);
    packet.push_back((length >> 16) & 0xFF);
    packet.push_back((length >> 24) & 0xFF);

    packet.insert(packet.end(), data.begin(), data.end()); //finally, we insert the entire payload

    return packet;
}

bool Discord_IPC_handler::connect_to_socket()
{
    std::string socket_path; //full filepath to the unix socket that discord uses for IPC
    //attempt to try different socket paths
    const char* tmp_dir = getenv("XDG_RUNTIME_DIR");
    const char* try_dirs[] = { getenv("TMPDIR"), getenv("TMP"), getenv("TEMP")}; //all the possible environment variables that may house the Discord IPC socket, usually XDG_RUNTIME_DIR, this should *hopefully* also work for MAC

    for (int i=0; i != sizeof(try_dirs)/sizeof(try_dirs[0]); i++) //attempting to get the directory for Discord's Unix socket
    {
        if (tmp_dir != NULL)
        {
            break;
        }
        else
        {
            tmp_dir = try_dirs[i];
        }
    }
    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1)
    {
        return false;
    }

    struct sockaddr_un address;
    memset(&address, 0, sizeof(address)); //clearing/initialising our structure
    address.sun_family = AF_UNIX;

    for (int pipe = 0; pipe < 10; ++pipe) // Try to connect to Discord IPC pipes (0-9)
    {
        socket_path = std::string(tmp_dir) + "/discord-ipc-" + std::to_string(pipe);
        strncpy(address.sun_path, socket_path.c_str(), sizeof(address.sun_path) - 1);

        if (connect(fd, (struct sockaddr*)&address, sizeof(address)) != -1)
        {
            std::cout << "Connected to Discord IPC pipe " << pipe << std::endl;
            return true;
        }
        else
        {
            close(fd); //we disconnect if we cant find any socket (Discord likely not running)
            fd = -1;
        }
    }
    return false;
}

bool Discord_IPC_handler::send_handshake()
{
    std::string handshake_json = R"({"v":1,"client_id":")" + application_ID + R"("})"; //this handshake message tells Discord the protocol we are using (v1) and the application ID of our app, R"" (raw string literal) used to easily add '"' character
    auto packet = pack_message(OpCode::HANDSHAKE, handshake_json);

    ssize_t sent = send(fd, packet.data(), packet.size(), 0); //sends our handshake packet to the Discord socket
    return sent == static_cast<ssize_t>(packet.size());
}

bool Discord_IPC_handler::read_response()
{
    uint8_t header[8];
    ssize_t received = recv(fd, header, sizeof(header), 0);

    if (received != sizeof(header))
    {
        return false;
    }

    //extract opcode and length
    uint32_t opcode = header[0] | (header[1] << 8) | (header[2] << 16) | (header[3] << 24);
    uint32_t length = header[4] | (header[5] << 8) | (header[6] << 16) | (header[7] << 24);

    if (length > 0)
    {
        std::vector<uint8_t> received_data(length);
        received = recv(fd, received_data.data(), length, 0); //receives data from the Discord socket and puts it into our data vector, the length of the received packet is put into our length variable
        if (received != static_cast<ssize_t>(length))
        {
            return false;
        }
        std::string response(received_data.begin(), received_data.end()); //the actual data (payload) Discord sent back to us
        std::cout << "Discord response: " << response << std::endl;
    }
    return true;
}
