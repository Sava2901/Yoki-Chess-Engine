#ifndef SMALLVECTOR_H
#define SMALLVECTOR_H

#include <array>
#include <vector>
#include <algorithm>
#include <iterator>

/**
 * @brief Small vector optimization for chess moves
 * 
 * Uses a fixed-size buffer for typical move counts (~32 moves) to avoid
 * heap allocations in the common case. Falls back to std::vector for
 * larger collections (max legal moves ~218 in chess).
 * 
 * @tparam T Element type
 * @tparam N Size of the fixed buffer
 */
template<typename T, size_t N = 32>
class SmallVector {
private:
    std::array<T, N> buffer_;           // Fixed-size buffer for small collections
    std::vector<T> overflow_;           // Heap storage for larger collections
    size_t size_ = 0;                   // Current number of elements
    bool using_overflow_ = false;       // Whether we're using heap storage

public:
    using value_type = T;
    using size_type = size_t;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    
    // Iterator types
    using iterator = T*;
    using const_iterator = const T*;

    /**
     * @brief Default constructor
     */
    SmallVector() = default;

    /**
     * @brief Constructor with initial capacity reservation
     * @param capacity Initial capacity to reserve
     */
    explicit SmallVector(size_t capacity) {
        if (capacity > N) {
            overflow_.reserve(capacity);
            using_overflow_ = true;
        }
    }

    /**
     * @brief Copy constructor
     */
    SmallVector(const SmallVector& other) 
        : size_(other.size_), using_overflow_(other.using_overflow_) {
        if (using_overflow_) {
            overflow_ = other.overflow_;
        } else {
            std::copy(other.buffer_.begin(), other.buffer_.begin() + size_, buffer_.begin());
        }
    }

    /**
     * @brief Move constructor
     */
    SmallVector(SmallVector&& other) noexcept
        : size_(other.size_), using_overflow_(other.using_overflow_) {
        if (using_overflow_) {
            overflow_ = std::move(other.overflow_);
        } else {
            std::move(other.buffer_.begin(), other.buffer_.begin() + size_, buffer_.begin());
        }
        other.clear();
    }

    /**
     * @brief Copy assignment operator
     */
    SmallVector& operator=(const SmallVector& other) {
        if (this != &other) {
            clear();
            size_ = other.size_;
            using_overflow_ = other.using_overflow_;
            if (using_overflow_) {
                overflow_ = other.overflow_;
            } else {
                std::copy(other.buffer_.begin(), other.buffer_.begin() + size_, buffer_.begin());
            }
        }
        return *this;
    }

    /**
     * @brief Move assignment operator
     */
    SmallVector& operator=(SmallVector&& other) noexcept {
        if (this != &other) {
            clear();
            size_ = other.size_;
            using_overflow_ = other.using_overflow_;
            if (using_overflow_) {
                overflow_ = std::move(other.overflow_);
            } else {
                std::move(other.buffer_.begin(), other.buffer_.begin() + size_, buffer_.begin());
            }
            other.clear();
        }
        return *this;
    }

    /**
     * @brief Add element to the end
     */
    void push_back(const T& value) {
        if (using_overflow_) {
            overflow_.push_back(value);
        } else if (size_ < N) {
            buffer_[size_] = value;
        } else {
            // Transition to overflow storage
            overflow_.reserve(N * 2);  // Reserve more space
            overflow_.assign(buffer_.begin(), buffer_.begin() + N);
            overflow_.push_back(value);
            using_overflow_ = true;
        }
        ++size_;
    }

    /**
     * @brief Add element to the end (move version)
     */
    void push_back(T&& value) {
        if (using_overflow_) {
            overflow_.push_back(std::move(value));
        } else if (size_ < N) {
            buffer_[size_] = std::move(value);
        } else {
            // Transition to overflow storage
            overflow_.reserve(N * 2);
            overflow_.assign(std::make_move_iterator(buffer_.begin()), 
                           std::make_move_iterator(buffer_.begin() + N));
            overflow_.push_back(std::move(value));
            using_overflow_ = true;
        }
        ++size_;
    }

    /**
     * @brief Construct element in place at the end
     */
    template<typename... Args>
    void emplace_back(Args&&... args) {
        if (using_overflow_) {
            overflow_.emplace_back(std::forward<Args>(args)...);
        } else if (size_ < N) {
            new (&buffer_[size_]) T(std::forward<Args>(args)...);
        } else {
            // Transition to overflow storage
            overflow_.reserve(N * 2);
            overflow_.assign(std::make_move_iterator(buffer_.begin()), 
                           std::make_move_iterator(buffer_.begin() + N));
            overflow_.emplace_back(std::forward<Args>(args)...);
            using_overflow_ = true;
        }
        ++size_;
    }

    /**
     * @brief Reserve capacity
     */
    void reserve(size_t capacity) {
        if (capacity > N && !using_overflow_) {
            // Transition to overflow storage
            overflow_.reserve(capacity);
            overflow_.assign(buffer_.begin(), buffer_.begin() + size_);
            using_overflow_ = true;
        } else if (using_overflow_) {
            overflow_.reserve(capacity);
        }
    }

    /**
     * @brief Clear all elements
     */
    void clear() {
        size_ = 0;
        if (using_overflow_) {
            overflow_.clear();
            using_overflow_ = false;
        }
    }

    /**
     * @brief Get current size
     */
    size_t size() const { return size_; }

    /**
     * @brief Check if empty
     */
    bool empty() const { return size_ == 0; }

    /**
     * @brief Get current capacity
     */
    size_t capacity() const {
        return using_overflow_ ? overflow_.capacity() : N;
    }

    /**
     * @brief Access element by index
     */
    reference operator[](size_t index) {
        return using_overflow_ ? overflow_[index] : buffer_[index];
    }

    /**
     * @brief Access element by index (const)
     */
    const_reference operator[](size_t index) const {
        return using_overflow_ ? overflow_[index] : buffer_[index];
    }

    /**
     * @brief Get reference to first element
     */
    reference front() {
        return using_overflow_ ? overflow_.front() : buffer_[0];
    }

    /**
     * @brief Get const reference to first element
     */
    const_reference front() const {
        return using_overflow_ ? overflow_.front() : buffer_[0];
    }

    /**
     * @brief Get reference to last element
     */
    reference back() {
        return using_overflow_ ? overflow_.back() : buffer_[size_ - 1];
    }

    /**
     * @brief Get const reference to last element
     */
    const_reference back() const {
        return using_overflow_ ? overflow_.back() : buffer_[size_ - 1];
    }

    /**
     * @brief Get iterator to beginning
     */
    iterator begin() {
        return using_overflow_ ? overflow_.data() : buffer_.data();
    }

    /**
     * @brief Get const iterator to beginning
     */
    const_iterator begin() const {
        return using_overflow_ ? overflow_.data() : buffer_.data();
    }

    /**
     * @brief Get iterator to end
     */
    iterator end() {
        return begin() + size_;
    }

    /**
     * @brief Get const iterator to end
     */
    const_iterator end() const {
        return begin() + size_;
    }

    /**
     * @brief Get const iterator to beginning
     */
    const_iterator cbegin() const { return begin(); }

    /**
     * @brief Get const iterator to end
     */
    const_iterator cend() const { return end(); }

    /**
     * @brief Insert range of elements at position
     */
    template<typename InputIt>
    iterator insert(iterator pos, InputIt first, InputIt last) {
        size_t pos_index = pos - begin();
        size_t insert_count = std::distance(first, last);
        
        if (insert_count == 0) {
            return begin() + pos_index;
        }
        
        // Check if we need to transition to overflow
        if (!using_overflow_ && size_ + insert_count > N) {
            // Transition to overflow storage
            overflow_.reserve(size_ + insert_count);
            overflow_.assign(buffer_.begin(), buffer_.begin() + size_);
            using_overflow_ = true;
        }
        
        if (using_overflow_) {
            auto it = overflow_.insert(overflow_.begin() + pos_index, first, last);
            size_ += insert_count;
            return overflow_.data() + (it - overflow_.begin());
        } else {
            // Insert in buffer - move existing elements
            std::move_backward(buffer_.begin() + pos_index, buffer_.begin() + size_, 
                             buffer_.begin() + size_ + insert_count);
            std::copy(first, last, buffer_.begin() + pos_index);
            size_ += insert_count;
            return buffer_.data() + pos_index;
        }
    }

    /**
     * @brief Erase range of elements
     */
    iterator erase(iterator first, iterator last) {
        if (first == last) {
            return first;
        }
        
        size_t first_index = first - begin();
        size_t last_index = last - begin();
        size_t erase_count = last_index - first_index;
        
        if (using_overflow_) {
            auto it = overflow_.erase(overflow_.begin() + first_index, 
                                    overflow_.begin() + last_index);
            size_ -= erase_count;
            return overflow_.data() + (it - overflow_.begin());
        } else {
            // Erase from buffer - move remaining elements
            std::move(buffer_.begin() + last_index, buffer_.begin() + size_, 
                     buffer_.begin() + first_index);
            size_ -= erase_count;
            return buffer_.data() + first_index;
        }
    }

    /**
     * @brief Erase single element
     */
    iterator erase(iterator pos) {
        return erase(pos, pos + 1);
    }
};

#endif // SMALLVECTOR_H