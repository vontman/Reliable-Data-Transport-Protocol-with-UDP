#ifndef SERVER_HEADER
#define SERVER_HEADER
#include <string>
#include <unordered_map>

class AckPacket;
class DataPacket;
class FileRequest;
class sockaddr_in;

class Server {
  public:
    const int MAX_CLIENTS = 30;
    const int PACKET_LEN = 500;

    explicit Server(int port, int max_window_size);
    void start();
    void stop();
    ~Server();

  private:
    void init_flag();

    int const d_port;
    int const d_max_window_size;

    bool d_is_running = false;
    std::unordered_map<std::string, std::pair<int, int>> pipes_mapper;
    // pipes[id].first for read end
    // pipes[id].second for write end

    int d_socket;

    void handle_file_request(sockaddr_in const &client_address,
                             FileRequest const &packet);
    void handle_ack_packet(sockaddr_in const &client_address,
                           AckPacket const &packet);
    void child_handle_client(sockaddr_in const &client_address,
                             std::pair<int, int> pipes,
                             FileRequest const &file_request);
};
#endif /* ifndef SERVER_HEADER */
