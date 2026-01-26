#include "common/MarketData.h"
#include "common/SharedMemory.h"
#include "common/Logger.h"

#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include <cstring>
#include <algorithm>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

constexpr int PORT = 9090;
constexpr size_t SHM_BUFFER_SIZE = 1024;
const std::string SHM_NAME = "/market_data_shm";

class TcpServer {
public:
    TcpServer(int port) {
        server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd_ == -1) {
            throw std::runtime_error("Socket creation failed");
        }

        int opt = 1;
        if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
            throw std::runtime_error("setsockopt SO_REUSEADDR failed");
        }

        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
            throw std::runtime_error("Bind failed");
        }

        if (listen(server_fd_, 10) < 0) {
            throw std::runtime_error("Listen failed");
        }

        int flags = fcntl(server_fd_, F_GETFL, 0);
        fcntl(server_fd_, F_SETFL, flags | O_NONBLOCK);

        Logger::Log("TCP Server listening on port {}", port);
    }

    ~TcpServer() {
        close(server_fd_);
        for (int client : clients_) {
            close(client);
        }
    }

    void accept_new_clients() {
        struct sockaddr_in address;
        int addrlen = sizeof(address);
        int new_socket = accept(server_fd_, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        
        if (new_socket >= 0) {
            int flag = 1;
            setsockopt(new_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));
            
            int flags = fcntl(new_socket, F_GETFL, 0);
            fcntl(new_socket, F_SETFL, flags | O_NONBLOCK);

            clients_.push_back(new_socket);
            Logger::Log("New client connected: {}", new_socket);
        }
    }

    void broadcast(const std::string& message) {
        std::string payload = message + "\n";
        
        for (auto it = clients_.begin(); it != clients_.end(); ) {
            int sock = *it;
            ssize_t sent = send(sock, payload.c_str(), payload.size(), 0); 
            
            if (sent == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    ++it; 
                } else {
                    Logger::Log("Client disconnected: {}", sock);
                    close(sock);
                    it = clients_.erase(it);
                }
            } else {
                ++it;
            }
        }
    }

private:
    int server_fd_;
    std::vector<int> clients_;
};

int main() {
    try {
        size_t shm_size = sizeof(MarketData::RingBuffer<SHM_BUFFER_SIZE>);
        SharedMemory shm_obj(SHM_NAME, shm_size, true); 
        
        auto* ring_buffer = new (shm_obj.get()) MarketData::RingBuffer<SHM_BUFFER_SIZE>();
        
        Logger::Log("Shared Memory initialized: {}", SHM_NAME);

        TcpServer server(PORT);

        std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<double> dist(-0.5, 0.5);
        double price = 2850.00;

        Logger::Log("Publisher started. Press Ctrl+C to stop.");
        
        while (true) {
            price += dist(rng);
            if (price < 100.0) price = 100.0; 

            double bid = price - 0.05;
            double ask = price + 0.05;

            MarketData::Message msg;
            std::strncpy(msg.instrument, "RELIANCE", sizeof(msg.instrument) - 1);
            msg.instrument[sizeof(msg.instrument) - 1] = '\0';
            msg.bid = bid;
            msg.ask = ask;
            msg.timestamp_ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();

            if (!ring_buffer->push(msg)) {
            }

            json j;
            j["instrument"] = msg.instrument;
            j["bid"] = msg.bid;
            j["ask"] = msg.ask;
            j["timestamp_ns"] = msg.timestamp_ns;
            
            server.broadcast(j.dump());
            server.accept_new_clients();

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

    } catch (const std::exception& e) {
        Logger::Log("Publisher Error: {}", e.what());
        return 1;
    }

    return 0;
}
