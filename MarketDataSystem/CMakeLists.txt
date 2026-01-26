cmake_minimum_required(VERSION 3.14)
project(MarketDataSystem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG        10.1.1 
)
FetchContent_MakeAvailable(fmt)

FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)

include_directories(include)

add_executable(publisher src/publisher/main.cpp)
target_link_libraries(publisher PRIVATE fmt::fmt nlohmann_json::nlohmann_json)

add_executable(consumer_shm src/consumer_shm/main.cpp)
target_link_libraries(consumer_shm PRIVATE fmt::fmt nlohmann_json::nlohmann_json)

add_executable(consumer_tcp src/consumer_tcp/main.cpp)
target_link_libraries(consumer_tcp PRIVATE fmt::fmt nlohmann_json::nlohmann_json)
