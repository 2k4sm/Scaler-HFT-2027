#include "common/Logger.h"

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/tcp.h>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

constexpr int PORT = 9090;

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[4096] = {0};

    try {
        while (true) {
            sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0) {
                Logger::Log("Socket creation error");
                return -1;
            }

            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(PORT);

            if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
                Logger::Log("Invalid address/ Address not supported");
                return -1;
            }

            if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                close(sock);
                std::this_thread::sleep_for(std::chrono::seconds(1));
            } else {
                Logger::Log("Connected to TCP Publisher");
                break;
            }
        }

        std::string accumulated_data;
        while (true) {
            ssize_t valread = read(sock, buffer, 4096);
            if (valread > 0) {
                accumulated_data.append(buffer, valread);
                
                size_t pos = 0;
                while ((pos = accumulated_data.find('\n')) != std::string::npos) {
                    std::string line = accumulated_data.substr(0, pos);
                    accumulated_data.erase(0, pos + 1);

                    try {
                        auto j = json::parse(line);
                        std::string instrument = j["instrument"];
                        double bid = j["bid"];
                        double ask = j["ask"];

                        Logger::Log("{} BID={:.2f} ASK={:.2f}", instrument, bid, ask);
                    } catch (const std::exception& e) {
                        Logger::Log("JSON Parse Error: {}", e.what());
                    }
                }
            } else if (valread == 0) {
                Logger::Log("Server disconnected");
                break;
            } else {
                Logger::Log("Read error");
                break;
            }
        }

        close(sock);
    } catch (const std::exception& e) {
        Logger::Log("TCP Consumer Error: {}", e.what());
        return 1;
    }

    return 0;
}
