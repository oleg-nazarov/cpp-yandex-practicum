#pragma once

#include <algorithm>
#include <cassert>
#include <iterator>
#include <memory>
#include <new>
#include <utility>

template <typename T>
class RawMemory {
   public:
    RawMemory() = default;
    explicit RawMemory(size_t capacity) : buffer_(Allocate(capacity)), capacity_(capacity) {}
    RawMemory(const RawMemory&) = delete;
    RawMemory(RawMemory&& other) noexcept {
        *this = std::move(other);
    }

    ~RawMemory() {
        if (buffer_ != nullptr) {
            Deallocate(buffer_);
        }
    }

    RawMemory& operator=(const RawMemory& rhs) = delete;
    RawMemory& operator=(RawMemory&& rhs) noexcept {
        if (GetAddress() != rhs.GetAddress()) {
            buffer_ = std::exchange(rhs.buffer_, nullptr);
            capacity_ = std::exchange(rhs.capacity_, 0);
        }

        return *this;
    }

    T* operator+(size_t offset) noexcept {
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

   private:
    T* buffer_ = nullptr;
    size_t capacity_ = 0;

    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }
};

template <typename T>
class Vector {
   public:
    using iterator = T*;
    using const_iterator = const T*;

    Vector() = default;
    explicit Vector(size_t size) : data_(size), size_(size) {
        std::uninitialized_value_construct_n(data_.GetAddress(), size);
    }
    Vector(Vector&& other) noexcept {
        if (this != &other) {
            Swap(other);
        }
    }
    Vector(const Vector& other) : data_(other.size_), size_(other.size_) {
        if (this != &other) {
            std::uninitialized_copy_n(other.data_.GetAddress(), other.size_, data_.GetAddress());
        }
    }

    ~Vector() {
        std::destroy_n(data_.GetAddress(), size_);
    }

    Vector& operator=(Vector&& rhs) noexcept {
        if (this != &rhs) {
            Swap(rhs);
        }

        return *this;
    }
    Vector& operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (rhs.size_ > data_.Capacity()) {
                Vector<T> rhs_copy(rhs);
                Swap(rhs_copy);
            } else {
                if (size_ < rhs.size_) {
                    std::copy(rhs.data_.GetAddress(), rhs.data_.GetAddress() + size_, data_.GetAddress());
                    std::uninitialized_copy_n(rhs.data_.GetAddress() + size_, rhs.size_ - size_, data_.GetAddress() + size_);
                } else {
                    std::copy(rhs.data_.GetAddress(), rhs.data_.GetAddress() + rhs.size_, data_.GetAddress());
                    std::destroy_n(data_.GetAddress() + rhs.size_, size_ - rhs.size_);
                }

                size_ = rhs.size_;
            }
        }

        return *this;
    }

    void Resize(size_t new_size) {
        if (new_size > Capacity()) {
            Reserve(new_size);
        }

        if (new_size > size_) {
            std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
        } else {
            std::destroy_n(data_.GetAddress() + new_size, size_ - new_size);
        }

        size_ = new_size;
    }

    template <typename Val>
    void PushBack(Val&& value) {
        EmplaceBack(std::forward<Val>(value));
    }

    void PopBack() {
        if (size_ == 0u) {
            return;
        }

        std::destroy_n(data_.GetAddress() + (size_ - 1u), 1);
        --size_;
    }

    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        iterator it = Emplace(end(), std::forward<Args>(args)...);
        return *it;
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        size_t offset = pos - cbegin();

        if (size_ == Capacity()) {
            RawMemory<T> new_data(GetDoubledCapacity());
            new (new_data + offset) T(std::forward<Args>(args)...);

            try {
                ShiftDataToNewMemory(data_.GetAddress(), offset, new_data.GetAddress());
            } catch (...) {
                std::destroy_n(new_data.GetAddress() + offset, 1);
                throw;
            }

            try {
                ShiftDataToNewMemory(data_.GetAddress() + offset, size_ - offset, new_data.GetAddress() + (offset + 1));
            } catch (...) {
                std::destroy_n(new_data.GetAddress(), offset + 1);
                throw;
            }

            std::destroy_n(data_.GetAddress(), size_);

            data_.Swap(new_data);
        } else {
            if (pos == cend()) {
                new (data_ + offset) T(std::forward<Args>(args)...);
            } else {
                T temp_val(std::forward<Args>(args)...);

                new (end()) T(std::move(*(end() - 1)));
                std::move_backward(begin() + offset, end() - 1, end());
                data_[offset] = std::move(temp_val);
            }
        }

        ++size_;

        return begin() + offset;
    }

    iterator Erase(const_iterator pos) {
        size_t offset = pos - cbegin();

        std::move(begin() + offset + 1, end(), begin() + offset);
        std::destroy_n(end() - 1, 1);
        --size_;

        return begin() + offset;
    }

    iterator Insert(const_iterator pos, const T& value) {
        return Emplace(pos, value);
    }
    iterator Insert(const_iterator pos, T&& value) {
        return Emplace(pos, std::move(value));
    }

    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }

        RawMemory<T> new_data(new_capacity);
        ShiftDataToNewMemory(data_.GetAddress(), size_, new_data.GetAddress());
        std::destroy_n(data_.GetAddress(), size_);

        data_.Swap(new_data);
    }

    iterator begin() noexcept {
        return const_cast<iterator>(cbegin());
    }
    iterator end() noexcept {
        return const_cast<iterator>(cend());
    }
    const_iterator begin() const noexcept {
        return data_.GetAddress();
    }
    const_iterator end() const noexcept {
        return data_.GetAddress() + size_;
    }
    const_iterator cbegin() const noexcept {
        return begin();
    }
    const_iterator cend() const noexcept {
        return end();
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

   private:
    RawMemory<T> data_;
    size_t size_ = 0;

    size_t GetDoubledCapacity() {
        return size_ == 0u ? 1 : size_ * 2;
    }

    void ShiftDataToNewMemory(T* old_buf, size_t count, T* new_buf) {
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(old_buf, count, new_buf);
        } else {
            std::uninitialized_copy_n(old_buf, count, new_buf);
        }
    }
};
