#include "common/MarketData.h"
#include "common/SharedMemory.h"
#include "common/Logger.h"

#include <thread>
#include <chrono>
#include <iostream>

constexpr size_t SHM_BUFFER_SIZE = 1024;
const std::string SHM_NAME = "/market_data_shm";

int main() {
    try {
        Logger::Log("SHM Consumer waiting for Shared Memory...");

        SharedMemory* shm_ptr = nullptr;
        while (true) {
            try {
                shm_ptr = new SharedMemory(SHM_NAME, sizeof(MarketData::RingBuffer<SHM_BUFFER_SIZE>), false);
                break;
            } catch (const std::exception&) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        std::unique_ptr<SharedMemory> shm_obj(shm_ptr);

        auto* ring_buffer = reinterpret_cast<MarketData::RingBuffer<SHM_BUFFER_SIZE>*>(shm_obj->get());

        Logger::Log("Connected to Shared Memory. Listening for updates...");

        MarketData::Message msg;
        while (true) {
            if (ring_buffer->pop(msg)) {
                Logger::Log("{} BID={:.2f} ASK={:.2f}", msg.instrument, msg.bid, msg.ask);
            } else {
                std::this_thread::yield(); 
            }
        }

    } catch (const std::exception& e) {
        Logger::Log("SHM Consumer Error: {}", e.what());
        return 1;
    }

    return 0;
}
