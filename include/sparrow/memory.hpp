#include <stdexcept>
#include <memory>

namespace sparrow {

template <class T>
class value_ptr {
public:
    value_ptr() = default;

    explicit value_ptr(T value)
        : value_(std::make_unique<T>(std::move(value))) {}

    value_ptr(const value_ptr& other)
        : value_(other.value_ ? std::make_unique<T>(*other.value_) : std::unique_ptr<T>()){}

    value_ptr(value_ptr&& other) noexcept
        : value_(std::move(other.value_)) {
        other.reset();
    }

    value_ptr& operator=(const value_ptr& other) {
        if (other.has_value()) {
            value_ = std::make_unique<T>(*other.value_);
        }else {
            value_.reset();
        }
        return *this;
    }

    value_ptr& operator=(value_ptr&& other) noexcept {
        if (this != &other) {
            value_ = std::move(other.value_);
            other.reset();
        }
        return *this;
    }

    T& operator*() {
        if (!value_) {
            throw std::runtime_error("No value stored in value_ptr");
        }
        return *value_;
    }

    const T& operator*() const {
        if (!value_) {
            throw std::runtime_error("No value stored in value_ptr");
        }
        return *value_;
    }

    T* operator->() {
        if (!value_) {
            throw std::runtime_error("No value stored in value_ptr");
        }
        return &*value_;
    }

    const T* operator->() const {
        if (!value_) {
            throw std::runtime_error("No value stored in value_ptr");
        }
        return &*value_;
    }

    explicit operator bool() const noexcept {
        return has_value();
    }

    bool has_value() const noexcept {
        return bool(value_);
    }

    void reset() noexcept {
        value_.reset();
    }

private:
    std::unique_ptr<T> value_;
};

}
