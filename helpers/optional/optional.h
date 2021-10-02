#include <iterator>
#include <stdexcept>
#include <utility>

class BadOptionalAccess : public std::exception {
   public:
    using exception::exception;

    virtual const char* what() const noexcept override {
        return "Bad optional access";
    }
};

template <typename T>
class Optional {
   public:
    Optional() = default;
    template <typename Value>
    Optional(Value&& value) {
        *this = std::forward<Value>(value);
    }

    ~Optional() {
        Reset();
    }

    template <typename... Values>
    T& Emplace(Values&&... values) {
        if (HasValue()) {
            CallDestructorOfData();
        } else {
            is_initialized_ = true;
        }

        new (&data_[0]) T(std::forward<Values>(values)...);

        return Value();
    }

    Optional& operator=(const T& value) {
        if (HasValue()) {
            T& cur_value = Value();
            cur_value = value;
        } else {
            new (&data_[0]) T(value);
            is_initialized_ = true;
        }

        return *this;
    }
    Optional& operator=(T&& value) {
        if (HasValue()) {
            T& cur_value = Value();
            cur_value = std::move(value);
        } else {
            new (&data_[0]) T(std::move(value));
            is_initialized_ = true;
        }

        return *this;
    }

    Optional& operator=(const Optional& rhs) {
        if (rhs.HasValue()) {
            *this = rhs.Value();
        } else {
            this->Reset();
        }

        return *this;
    }
    Optional& operator=(Optional&& rhs) {
        if (rhs.HasValue()) {
            *this = std::move(rhs.Value());
        } else {
            this->Reset();
        }

        return *this;
    }

    bool HasValue() const {
        return is_initialized_;
    }

    T& operator*() & {
        return reinterpret_cast<T&>(data_);
    }
    T&& operator*() && {
        return reinterpret_cast<T&&>(data_);
    }
    const T& operator*() const& {
        return reinterpret_cast<const T&>(data_);
    }

    T* operator->() {
        return reinterpret_cast<T*>(data_);
    }
    const T* operator->() const {
        return reinterpret_cast<const T*>(data_);
    }

    T&& Value() && {
        if (!is_initialized_) {
            throw BadOptionalAccess();
        }

        return reinterpret_cast<T&&>(data_);
    }
    T& Value() & {
        if (!is_initialized_) {
            throw BadOptionalAccess();
        }

        return reinterpret_cast<T&>(data_);
    }
    const T& Value() const& {
        if (!HasValue()) {
            throw BadOptionalAccess();
        }

        return reinterpret_cast<const T&>(data_);
    }

    void Reset() {
        if (HasValue()) {
            CallDestructorOfData();
            is_initialized_ = false;
        }
    }

   private:
    alignas(T) char data_[sizeof(T)];
    bool is_initialized_ = false;

    void CallDestructorOfData() {
        reinterpret_cast<T*>(data_)->~T();
    }
};
