#include "megamic/io/MachineCommPort.h"
#include <chrono>

namespace megamic {
namespace io {

MachineCommPort::MachineCommPort(simulator::Machine* machine, std::shared_ptr<CommChannel> channel)
    : machine_(machine), channel_(channel) {
}

MachineCommPort::~MachineCommPort() {
}

void MachineCommPort::queueException(uint8_t exceptionCode) {
    std::lock_guard<std::recursive_mutex> lock(exceptionMutex_);
    exceptionQueue_.push(Exception(exceptionCode, getCurrentTimestamp()));
    exceptionCondition_.notify_one();
}

void MachineCommPort::clearExceptions() {
    std::lock_guard<std::recursive_mutex> lock(exceptionMutex_);
    while (!exceptionQueue_.empty()) {
        exceptionQueue_.pop();
    }
}

bool MachineCommPort::hasExceptions() const {
    std::lock_guard<std::recursive_mutex> lock(exceptionMutex_);
    return !exceptionQueue_.empty();
}

uint64_t MachineCommPort::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

} // namespace io
} // namespace megamic
