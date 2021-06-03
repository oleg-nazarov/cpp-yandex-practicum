#include <algorithm>
#include <cstdlib>

template <typename Type>
class ArrayPtr {
   public:
    ArrayPtr() = default;

    explicit ArrayPtr(size_t size) {
        raw_ptr_ = size == 0u ? nullptr : new Type[size];
    }

    explicit ArrayPtr(Type* raw_ptr) noexcept {
        raw_ptr_ = raw_ptr;
        raw_ptr = nullptr;
    }

    ArrayPtr(const ArrayPtr&) = delete;

    ~ArrayPtr() {
        delete[] raw_ptr_;
    }

    ArrayPtr& operator=(const ArrayPtr&) = delete;

    [[nodiscard]] Type* Release() noexcept {
        Type* released = raw_ptr_;
        raw_ptr_ = nullptr;

        return released;
    }

    Type& operator[](size_t index) noexcept {
        return raw_ptr_[index];
    }

    const Type& operator[](size_t index) const noexcept {
        return raw_ptr_[index];
    }

    explicit operator bool() const {
        return raw_ptr_ != nullptr;
    }

    Type* Get() const noexcept {
        return raw_ptr_;
    }

    void swap(ArrayPtr& other) noexcept {
        std::swap(raw_ptr_, other.raw_ptr_);
    }

   private:
    Type* raw_ptr_ = nullptr;
};
