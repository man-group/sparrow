#include <cassert>
#include <memory>

namespace sparrow {

/**
 * @brief A value_ptr is a smart pointer that behaves like a value.
 * It manages the lifetime of an object of type T.
 * When copied, it copies the managed object.
 * 
 * @tparam T The type of the object managed by the `value_ptr`.
 */
template <class T>
class value_ptr {
public:
    value_ptr() = default;

    explicit value_ptr(T value)
        : value_(std::make_unique<T>(std::move(value))) {}

    explicit value_ptr(T* value) 
        : value_(value != nullptr ? std::make_unique<T>(*value) : std::unique_ptr<T>()) {}

    value_ptr(const value_ptr& other)
        : value_(other.value_ ? std::make_unique<T>(*other.value_) : std::unique_ptr<T>()){}

    value_ptr(value_ptr&& other) noexcept = default;

    ~value_ptr() = default;

    value_ptr& operator=(const value_ptr& other) {
        if (other.has_value()) {
            *value_ = *other.value_;
        }else {
            value_.reset();
        }
        return *this;
    }

    value_ptr& operator=(value_ptr&& other) noexcept = default;

    T& operator*() {
        assert(value_);
        return *value_;
    }

    const T& operator*() const {
        assert(value_);
        return *value_;
    }

    T* operator->() {
        assert(value_);
        return &*value_;
    }

    const T* operator->() const {
        assert(value_);
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
