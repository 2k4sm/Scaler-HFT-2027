#pragma once

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <stdexcept>
#include <iostream>

#include <cstring>
#include <cerrno>

class SharedMemory {
public:
    SharedMemory(const std::string& name, size_t size, bool create = false)
        : name_(name), size_(size), is_owner_(create) {
        
        if (create) {
            shm_unlink(name.c_str());
        }

        int flags = O_RDWR;
        if (create) {
            flags |= O_CREAT;
        }

        fd_ = shm_open(name.c_str(), flags, 0666);
        if (fd_ == -1) {
            throw std::runtime_error("shm_open failed: " + std::string(strerror(errno)));
        }

        if (create) {
            if (ftruncate(fd_, size) == -1) {
                std::string err = strerror(errno);
                close(fd_);
                throw std::runtime_error("ftruncate failed: " + err);
            }
        }

        ptr_ = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
        if (ptr_ == MAP_FAILED) {
            close(fd_);
            throw std::runtime_error("mmap failed");
        }
    }

    ~SharedMemory() {
        if (ptr_ != MAP_FAILED) {
            munmap(ptr_, size_);
        }
        if (fd_ != -1) {
            close(fd_);
        }
        if (is_owner_) {
            shm_unlink(name_.c_str());
        }
    }

    void* get() const { return ptr_; }
    size_t size() const { return size_; }

private:
    std::string name_;
    size_t size_;
    int fd_ = -1;
    void* ptr_ = MAP_FAILED;
    bool is_owner_;
};
