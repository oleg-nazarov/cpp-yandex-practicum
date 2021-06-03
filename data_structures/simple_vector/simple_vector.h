#pragma once

#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <string>

#include "../array_ptr.h"

class ReserveProxyObj {
   public:
    ReserveProxyObj(size_t capacity) : capacity_(capacity) {}

    size_t GetCapacity() const noexcept {
        return capacity_;
    }

   private:
    size_t capacity_;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector {
   public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    explicit SimpleVector(ReserveProxyObj obj) {
        Reserve(obj.GetCapacity());
    }

    SimpleVector(size_t size, const Type& value = Type()) {
        Reserve(size);

        std::fill(begin(), begin() + size, value);
        size_ = size;
    }

    SimpleVector(std::initializer_list<Type> init) {
        Reserve(init.size());

        std::copy(init.begin(), init.end(), begin());
        size_ = init.size();
    }

    SimpleVector(const SimpleVector& other) {
        SimpleVector<Type> temp;
        temp = other;

        this->swap(temp);
    }

    SimpleVector(SimpleVector&& other) {
        SimpleVector<Type> temp;
        temp = std::move(other);

        this->swap(temp);
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (*this != rhs) {
            if (rhs.size_ != 0u) {
                SimpleVector<Type> temp;
                temp.Reserve(rhs.capacity_);
                std::copy(rhs.begin(), rhs.end(), temp.begin());

                this->swap(temp);
            }

            size_ = rhs.size_;
        }

        return *this;
    }

    SimpleVector& operator=(SimpleVector&& rhs) {
        if (rhs.size_ != 0u) {
            SimpleVector<Type> temp;
            temp.Reserve(std::exchange(rhs.capacity_, 0));
            std::move(rhs.begin(), rhs.end(), temp.begin());

            this->swap(temp);
        }

        size_ = std::exchange(rhs.size_, 0);

        return *this;
    }

    size_t GetSize() const noexcept {
        return size_;
    }

    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    bool IsEmpty() const noexcept {
        return size_ == 0u;
    }

    Type& operator[](size_t index) noexcept {
        assert(index < size_);

        return items_[index];
    }

    const Type& operator[](size_t index) const noexcept {
        assert(index < size_);

        return items_[index];
    }

    Type& At(size_t index) {
        if (index >= size_) {
            ThrowIndexOutOfRange(index);
        }

        return items_[index];
    }

    const Type& At(size_t index) const {
        if (index >= size_) {
            ThrowIndexOutOfRange(index);
        }

        return items_[index];
    }

    void PushBack(const Type& value) {
        if (size_ == capacity_) {
            Reserve(GetDoubledCapacity());
        }

        items_[size_] = value;
        ++size_;
    }

    void PushBack(Type&& value) {
        if (size_ == capacity_) {
            Reserve(GetDoubledCapacity());
        }

        items_[size_] = std::move(value);
        ++size_;
    }

    Iterator Insert(ConstIterator pos, const Type& value) {
        Iterator non_const_pos = const_cast<Iterator>(pos);

        assert(non_const_pos >= items_.Get() && non_const_pos <= &items_[size_]);

        if (size_ == capacity_) {
            size_t idx = non_const_pos - items_.Get();

            Reserve(GetDoubledCapacity());
            non_const_pos = &items_[idx];
        }

        std::copy_backward(non_const_pos, &items_[size_], &items_[size_ + 1]);
        ++size_;
        *non_const_pos = value;

        return non_const_pos;
    }

    Iterator Insert(ConstIterator pos, Type&& value) {
        Iterator non_const_pos = const_cast<Iterator>(pos);

        assert(non_const_pos >= items_.Get() && non_const_pos <= &items_[size_]);

        if (size_ == capacity_) {
            size_t idx = non_const_pos - items_.Get();

            Reserve(GetDoubledCapacity());
            non_const_pos = &items_[idx];
        }

        std::move_backward(non_const_pos, &items_[size_], &items_[size_ + 1]);
        ++size_;
        *non_const_pos = std::move(value);

        return non_const_pos;
    }

    void PopBack() noexcept {
        assert(size_ > 0u);

        --size_;
    }

    Iterator Erase(ConstIterator pos) {
        Iterator non_const_pos = const_cast<Iterator>(pos);

        assert(non_const_pos >= items_.Get() && non_const_pos < &items_[size_]);

        std::move(non_const_pos + 1, &items_[size_ + 1], non_const_pos);
        --size_;

        return non_const_pos;
    }

    void Clear() noexcept {
        size_ = 0;
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= capacity_) {
            return;
        }

        ArrayPtr<Type> new_items(new Type[new_capacity]);
        std::move(items_.Get(), items_.Get() + size_, new_items.Get());

        items_.swap(new_items);
        capacity_ = new_capacity;
    }

    void Resize(size_t new_size) {
        if (new_size > capacity_) {
            Reserve(std::max(new_size, capacity_ * 2));
        }

        if (new_size > size_) {
            std::fill(begin() + size_, begin() + new_size, Type());
        }

        size_ = new_size;
    }

    void swap(SimpleVector& other) noexcept {
        this->items_.swap(other.items_);
        std::swap(this->size_, other.size_);
        std::swap(this->capacity_, other.capacity_);
    }

    Iterator begin() noexcept {
        return items_.Get();
    }

    Iterator end() noexcept {
        return items_.Get() + size_;
    }

    ConstIterator begin() const noexcept {
        return cbegin();
    }

    ConstIterator end() const noexcept {
        return cend();
    }

    ConstIterator cbegin() const noexcept {
        return items_.Get();
    }

    ConstIterator cend() const noexcept {
        return items_.Get() + size_;
    }

   private:
    ArrayPtr<Type> items_;
    size_t capacity_ = 0;
    size_t size_ = 0;

    size_t GetDoubledCapacity() noexcept {
        return capacity_ == 0u ? 1 : capacity_ * 2;
    }

    void ThrowIndexOutOfRange(size_t index) const {
        using namespace std::literals::string_literals;

        throw std::out_of_range("index "s + std::to_string(index) + " >= size_ "s + std::to_string(size_));
    }
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin()) && lhs.GetSize() == rhs.GetSize();
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return lhs < rhs || lhs == rhs;
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return lhs > rhs || lhs == rhs;
}
