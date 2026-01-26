#pragma once

#include <fmt/format.h>
#include <fmt/chrono.h>
#include <chrono>
#include <string_view>

namespace Logger {

    template <typename... Args>
    void Log(std::string_view format_str, Args&&... args) {
        auto now = std::chrono::system_clock::now();
        
        auto now_c = std::chrono::system_clock::to_time_t(now);
        auto duration = now.time_since_epoch();
        auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count() % 1000000000;

        std::tm tm_val;
        localtime_r(&now_c, &tm_val);

        fmt::print("[{:02}:{:02}:{:02}.{:09}] ", 
                   tm_val.tm_hour, tm_val.tm_min, tm_val.tm_sec, nanos);
        
        fmt::print(fmt::runtime(format_str), std::forward<Args>(args)...);
        fmt::print("\n");
    }
}
