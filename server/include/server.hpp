#ifndef SERVER_HEADER
#define SERVER_HEADER
#include <string>

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

    int d_socket;
};
#endif /* ifndef SERVER_HEADER */
