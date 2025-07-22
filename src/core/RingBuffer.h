#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <atomic>
#include <vector>
#include <cstring>
#include <algorithm>

template<typename T>
class RingBuffer {
public:
    explicit RingBuffer(size_t size) 
        : buffer_(size)
        , size_(size)
        , writePos_(0)
        , readPos_(0) {
    }
    
    bool write(const T* data, size_t count) {
        size_t available = getWriteAvailable();
        if (count > available) {
            return false; // Would overflow
        }
        
        size_t writePos = writePos_.load(std::memory_order_relaxed);
        size_t firstPart = std::min(count, size_ - writePos);
        size_t secondPart = count - firstPart;
        
        // Copy first part (up to end of buffer)
        std::memcpy(&buffer_[writePos], data, firstPart * sizeof(T));
        
        // Copy second part (wrap around to beginning)
        if (secondPart > 0) {
            std::memcpy(&buffer_[0], data + firstPart, secondPart * sizeof(T));
        }
        
        // Update write position
        writePos_.store((writePos + count) % size_, std::memory_order_release);
        
        return true;
    }
    
    bool read(T* data, size_t count) {
        size_t available = getReadAvailable();
        if (count > available) {
            return false; // Not enough data
        }
        
        size_t readPos = readPos_.load(std::memory_order_relaxed);
        size_t firstPart = std::min(count, size_ - readPos);
        size_t secondPart = count - firstPart;
        
        // Copy first part
        std::memcpy(data, &buffer_[readPos], firstPart * sizeof(T));
        
        // Copy second part
        if (secondPart > 0) {
            std::memcpy(data + firstPart, &buffer_[0], secondPart * sizeof(T));
        }
        
        // Update read position
        readPos_.store((readPos + count) % size_, std::memory_order_release);
        
        return true;
    }
    
    size_t getReadAvailable() const {
        size_t writePos = writePos_.load(std::memory_order_acquire);
        size_t readPos = readPos_.load(std::memory_order_relaxed);
        
        if (writePos >= readPos) {
            return writePos - readPos;
        } else {
            return size_ - readPos + writePos;
        }
    }
    
    size_t getWriteAvailable() const {
        size_t writePos = writePos_.load(std::memory_order_relaxed);
        size_t readPos = readPos_.load(std::memory_order_acquire);
        
        if (readPos > writePos) {
            return readPos - writePos - 1;
        } else {
            return size_ - writePos + readPos - 1;
        }
    }
    
    void reset() {
        writePos_.store(0, std::memory_order_relaxed);
        readPos_.store(0, std::memory_order_relaxed);
    }
    
    size_t size() const { return size_; }
    
private:
    std::vector<T> buffer_;
    const size_t size_;
    std::atomic<size_t> writePos_;
    std::atomic<size_t> readPos_;
};

// Specialization for complex float IQ data
using IQBuffer = RingBuffer<std::complex<float>>;

#endif // RINGBUFFER_H
