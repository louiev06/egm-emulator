#include "megamic/io/CommChannel.h"
#include <algorithm>
#include <thread>

namespace megamic {
namespace io {

PipedCommChannel::PipedCommChannel(const std::string& name)
    : name_(name),
      isOpen_(false),
      connectedChannel_(nullptr) {
}

PipedCommChannel::~PipedCommChannel() {
    close();
}

bool PipedCommChannel::open() {
    isOpen_ = true;
    return true;
}

void PipedCommChannel::close() {
    isOpen_ = false;
    inputBuffer_.clear();
}

bool PipedCommChannel::isOpen() const {
    return isOpen_;
}

int PipedCommChannel::read(uint8_t* buffer, int maxBytes,
                           std::chrono::milliseconds timeout) {
    if (!isOpen_) {
        return -1;
    }

    auto start = std::chrono::steady_clock::now();
    while (inputBuffer_.empty()) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);

        if (elapsed >= timeout) {
            return 0;  // Timeout
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    int bytesToRead = std::min(static_cast<int>(inputBuffer_.size()), maxBytes);
    std::copy(inputBuffer_.begin(), inputBuffer_.begin() + bytesToRead, buffer);
    inputBuffer_.erase(inputBuffer_.begin(), inputBuffer_.begin() + bytesToRead);

    return bytesToRead;
}

int PipedCommChannel::write(const uint8_t* buffer, int numBytes) {
    if (!isOpen_) {
        return -1;
    }

    // If connected to another channel, write to its input buffer
    if (connectedChannel_) {
        connectedChannel_->inputBuffer_.insert(
            connectedChannel_->inputBuffer_.end(),
            buffer,
            buffer + numBytes
        );
    }

    return numBytes;
}

void PipedCommChannel::flush() {
    // No-op for piped channel
}

std::string PipedCommChannel::getName() const {
    return name_;
}

void PipedCommChannel::connectTo(std::shared_ptr<PipedCommChannel> other) {
    connectedChannel_ = other;
}

} // namespace io
} // namespace megamic
