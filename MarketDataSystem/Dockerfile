FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    clang \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN rm -rf build && mkdir -p build && cd build && \
    cmake .. && \
    make -j$(nproc)

EXPOSE 9090

CMD ["./build/publisher"]
