/* Copyright (c) 2016 The Value Types Authors. All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
==============================================================================*/

#ifndef XYZ_INDIRECT_H
#define XYZ_INDIRECT_H

#include <cassert>
#include <compare>
#include <concepts>
#include <initializer_list>
#include <memory>
#include <type_traits>
#include <utility>

namespace xyz {

#ifndef XYZ_UNREACHABLE_DEFINED
#define XYZ_UNREACHABLE_DEFINED
[[noreturn]] inline void unreachable() {  // LCOV_EXCL_LINE
#if (__cpp_lib_unreachable >= 202202L)
  std::unreachable();  // LCOV_EXCL_LINE
#elif defined(_MSC_VER)
  __assume(false);  // LCOV_EXCL_LINE
#else
  __builtin_unreachable();  // LCOV_EXCL_LINE
#endif
}
#endif  // XYZ_UNREACHABLE_DEFINED

template <class T, class A>
class indirect;

template <class>
inline constexpr bool is_indirect_v = false;

template <class T, class A>
inline constexpr bool is_indirect_v<indirect<T, A>> = true;

template <class T, class A = std::allocator<T>>
class indirect {
  using allocator_traits = std::allocator_traits<A>;

 public:
  using value_type = T;
  using allocator_type = A;
  using pointer = typename allocator_traits::pointer;
  using const_pointer = typename allocator_traits::const_pointer;

  // The template type TT defers the constraint evaluation until the constructor
  // is instantiated.
  template <class TT = T>
  explicit constexpr indirect(std::allocator_arg_t, const A& alloc)
    requires(std::is_default_constructible_v<TT> &&
             std::is_copy_constructible_v<TT>)
      : alloc_(alloc) {
    p_ = construct_from(alloc_);
  }

  // The template type TT defers the constraint evaluation until the constructor
  // is instantiated.
  template <class TT = T>
  explicit constexpr indirect()
    requires(std::is_default_constructible_v<A> &&
             std::is_default_constructible_v<TT> &&
             std::is_copy_constructible_v<TT>)
      : indirect(std::allocator_arg, A{}) {}

  //  The template type TT defers the constraint evaluation until the
  //  constructor is instantiated.
  template <class U = T, class TT = T>
  explicit constexpr indirect(std::allocator_arg_t, const A& alloc, U&& u)
    requires(std::is_same_v<TT, std::remove_cvref_t<U>> &&
             std::is_default_constructible_v<TT> &&
             std::is_copy_constructible_v<TT> &&
             not std::is_same_v<std::remove_cvref_t<U>, std::in_place_t> &&
             not std::is_same_v<std::remove_cvref_t<U>, indirect>)
      : alloc_(alloc) {
    p_ = construct_from(alloc_, std::forward<U>(u));
  }

  //  The template type TT defers the constraint evaluation until the
  //  constructor is instantiated.
  template <class U = T, class TT = T>
  explicit constexpr indirect(U&& u)
    requires(std::is_same_v<TT, std::remove_cvref_t<U>> &&
             std::is_default_constructible_v<TT> &&
             std::is_copy_constructible_v<TT> &&
             not std::is_same_v<std::remove_cvref_t<U>, std::in_place_t> &&
             not std::is_same_v<std::remove_cvref_t<U>, indirect>)
      : indirect(std::allocator_arg, A{}, std::in_place_t{},
                 std::forward<U>(u)) {}

  //  The template type TT defers the constraint evaluation until the
  //  constructor is instantiated.
  template <class U = T, class TT = T>
  explicit constexpr indirect(std::allocator_arg_t, const A& alloc,
                              std::in_place_t, U&& u)
    requires(std::is_same_v<TT, std::remove_cvref_t<U>> &&
             std::is_default_constructible_v<TT> &&
             std::is_copy_constructible_v<TT> &&
             not std::is_same_v<std::remove_cvref_t<U>, indirect>)
      : alloc_(alloc) {
    p_ = construct_from(alloc_, std::forward<U>(u));
  }

  // The template type TT defers the constraint evaluation until the constructor
  // is instantiated.
  template <class... Us, class TT = T>
  explicit constexpr indirect(std::in_place_t, Us&&... us)
    requires(std::constructible_from<T, Us && ...> &&
             std::is_default_constructible_v<A> &&
             std::is_copy_constructible_v<TT>)
      : indirect(std::allocator_arg, A{}, std::in_place,
                 std::forward<Us>(us)...) {}

  constexpr indirect(std::allocator_arg_t, const A& alloc,
                     const indirect& other)
      : alloc_(alloc) {
    if (other.valueless_after_move()) {
      p_ = nullptr;
      return;
    }
    p_ = construct_from(alloc_, *other);
  }

  // The template type TT defers the constraint evaluation until the constructor
  // is instantiated.
  template <class... Us, class TT = T>
  explicit constexpr indirect(std::allocator_arg_t, const A& alloc,
                              std::in_place_t, Us&&... us)
    requires(std::constructible_from<T, Us && ...> &&
             std::is_copy_constructible_v<TT>)
      : alloc_(alloc) {
    p_ = construct_from(alloc_, std::forward<Us>(us)...);
  }

  // The template type TT defers the constraint evaluation until the constructor
  // is instantiated.
  template <class U = T, class TT = T, class... Us>
  explicit constexpr indirect(std::in_place_t, std::initializer_list<U> ilist,
                              Us&&... us)
    requires(std::is_default_constructible_v<A> &&
             std::is_constructible_v<TT, std::initializer_list<U>, Us...> &&
             std::is_copy_constructible_v<TT>)
      : indirect(std::allocator_arg, A{}, std::in_place, ilist,
                 std::forward<Us>(us)...) {}

  // The template type TT defers the constraint evaluation until the constructor
  // is instantiated.
  template <class U = T, class TT = T, class... Us>
  explicit constexpr indirect(std::allocator_arg_t, const A& alloc,
                              std::in_place_t, std::initializer_list<U> ilist,
                              Us&&... us)
    requires(std::is_constructible_v<TT, std::initializer_list<U>, Us...> &&
             std::is_copy_constructible_v<TT>)
      : alloc_(alloc) {
    p_ = construct_from(alloc_, ilist, std::forward<Us>(us)...);
  }

  constexpr indirect(const indirect& other)
      : indirect(std::allocator_arg,
                 allocator_traits::select_on_container_copy_construction(
                     other.alloc_),
                 other) {}

  constexpr indirect(
      std::allocator_arg_t, const A& alloc,
      indirect&& other) noexcept(allocator_traits::is_always_equal::value)
      : p_(nullptr), alloc_(alloc) {
    if constexpr (allocator_traits::is_always_equal::value) {
      std::swap(p_, other.p_);
    } else {
      if (alloc_ == other.alloc_) {
        std::swap(p_, other.p_);
      } else {
        if (!other.valueless_after_move()) {
          p_ = construct_from(alloc_, std::move(*other));
        } else {
          p_ = nullptr;
        }
      }
    }
  }

  constexpr indirect(indirect&& other) noexcept(
      allocator_traits::is_always_equal::value)
      : indirect(std::allocator_arg, other.alloc_, std::move(other)) {}

  constexpr ~indirect() { reset(); }

  constexpr indirect& operator=(const indirect& other) {
    if (this == &other) return *this;

    // Check to see if the allocators need to be updated.
    // We defer actually updating the allocator until later because it may be
    // needed to delete the current control block.
    bool update_alloc =
        allocator_traits::propagate_on_container_copy_assignment::value;

    if (other.valueless_after_move()) {
      reset();
    } else {
      if (std::is_copy_assignable_v<T> && !valueless_after_move() &&
          alloc_ == other.alloc_) {
        *p_ = *other;
      } else {
        // Constructing a new object could throw so we need to defer resetting
        // or updating allocators until this is done.
        auto tmp =
            construct_from(update_alloc ? other.alloc_ : alloc_, *other.p_);
        reset();
        p_ = tmp;
      }
    }
    if (update_alloc) {
      alloc_ = other.alloc_;
    }
    return *this;
  }

  constexpr indirect& operator=(indirect&& other) noexcept(
      allocator_traits::propagate_on_container_move_assignment::value ||
      allocator_traits::is_always_equal::value) {
    if (this == &other) return *this;

    // Check to see if the allocators need to be updated.
    // We defer actually updating the allocator until later because it may be
    // needed to delete the current control block.
    bool update_alloc =
        allocator_traits::propagate_on_container_move_assignment::value;

    if (other.valueless_after_move()) {
      reset();
    } else {
      if (alloc_ == other.alloc_) {
        std::swap(p_, other.p_);
        other.reset();
      } else {
        // Constructing a new object could throw so we need to defer resetting
        // or updating allocators until this is done.
        auto tmp = construct_from(update_alloc ? other.alloc_ : alloc_,
                                  std::move(*other.p_));
        reset();
        p_ = tmp;
      }
    }
    if (update_alloc) {
      alloc_ = other.alloc_;
    }
    return *this;
  }

  [[nodiscard]] constexpr const T& operator*() const& noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return *p_;
  }

  [[nodiscard]] constexpr T& operator*() & noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return *p_;
  }

  [[nodiscard]] constexpr T&& operator*() && noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return std::move(*p_);
  }

  [[nodiscard]] constexpr const T&& operator*() const&& noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return std::move(*p_);
  }

  [[nodiscard]] constexpr const_pointer operator->() const noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return p_;
  }

  [[nodiscard]] constexpr pointer operator->() noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return p_;
  }

  [[nodiscard]] constexpr bool valueless_after_move() const noexcept {
    return p_ == nullptr;
  }

  constexpr allocator_type get_allocator() const noexcept { return alloc_; }

  constexpr void swap(indirect& other) noexcept(
      std::allocator_traits<A>::propagate_on_container_swap::value ||
      std::allocator_traits<A>::is_always_equal::value) {
    if constexpr (allocator_traits::propagate_on_container_swap::value) {
      // If allocators move with their allocated objects, we can swap both.
      std::swap(alloc_, other.alloc_);
      std::swap(p_, other.p_);
      return;
    } else /* constexpr */ {
      if (alloc_ == other.alloc_) {
        std::swap(p_, other.p_);
      } else {
        unreachable();  // LCOV_EXCL_LINE
      }
    }
  }

  friend constexpr void swap(indirect& lhs,
                             indirect& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
  }

  template <class U, class AA>
  [[nodiscard]] friend constexpr bool operator==(
      const indirect<T, A>& lhs,
      const indirect<U, AA>& rhs) noexcept(noexcept(*lhs == *rhs)) {
    if (lhs.valueless_after_move()) {
      return rhs.valueless_after_move();
    }
    if (rhs.valueless_after_move()) {
      return false;
    }
    return *lhs == *rhs;
  }

  template <class U, class AA>
  [[nodiscard]] friend constexpr auto operator<=>(
      const indirect<T, A>& lhs,
      const indirect<U, AA>& rhs) noexcept(noexcept(*lhs <=> *rhs))
      -> std::compare_three_way_result_t<T, U> {
    if (lhs.valueless_after_move() || rhs.valueless_after_move()) {
      return !lhs.valueless_after_move() <=> !rhs.valueless_after_move();
    }
    return *lhs <=> *rhs;
  }

  template <class U>
  [[nodiscard]] friend constexpr bool operator==(
      const indirect<T, A>& lhs, const U& rhs) noexcept(noexcept(*lhs == rhs))
    requires(!is_indirect_v<U>)
  {
    if (lhs.valueless_after_move()) {
      return false;
    }
    return *lhs == rhs;
  }

  template <class U>
  [[nodiscard]] friend constexpr auto operator<=>(
      const indirect<T, A>& lhs, const U& rhs) noexcept(noexcept(*lhs <=> rhs))
      -> std::compare_three_way_result_t<T, U>
    requires(!is_indirect_v<U>)
  {
    if (lhs.valueless_after_move()) {
      return false <=> true;
    }
    return *lhs <=> rhs;
  }

 private:
  pointer p_;

#if defined(_MSC_VER)
  // https://devblogs.microsoft.com/cppblog/msvc-cpp20-and-the-std-cpp20-switch/#msvc-extensions-and-abi
  [[msvc::no_unique_address]] A alloc_;
#else
  [[no_unique_address]] A alloc_;
#endif

  constexpr void reset() noexcept {
    if (p_ == nullptr) return;
    destroy_with(alloc_, p_);
    p_ = nullptr;
  }

  template <typename... Ts>
  [[nodiscard]] constexpr static pointer construct_from(A alloc, Ts&&... ts) {
    pointer mem = allocator_traits::allocate(alloc, 1);
    try {
      allocator_traits::construct(alloc, std::to_address(mem),
                                  std::forward<Ts>(ts)...);
      return mem;
    } catch (...) {
      allocator_traits::deallocate(alloc, mem, 1);
      throw;
    }
  }

  constexpr static void destroy_with(A alloc, pointer p) {
    allocator_traits::destroy(alloc, std::to_address(p));
    allocator_traits::deallocate(alloc, p, 1);
  }
};

template <class T>
concept is_hashable = requires(T t) { std::hash<T>{}(t); };

template <typename Value>
indirect(Value) -> indirect<Value>;

template <typename Value>
indirect(std::in_place_t, Value) -> indirect<Value>;

template <typename Alloc, typename Value>
indirect(std::allocator_arg_t, Alloc, std::in_place_t, Value) -> indirect<
    Value, typename std::allocator_traits<Alloc>::template rebind_alloc<Value>>;

}  // namespace xyz

template <class T, class Alloc>
  requires xyz::is_hashable<T>
struct std::hash<xyz::indirect<T, Alloc>> {
  constexpr std::size_t operator()(const xyz::indirect<T, Alloc>& key) const {
    if (key.valueless_after_move()) {
      return static_cast<std::size_t>(-1);  // Implementation defined value.
    }
    return std::hash<typename xyz::indirect<T, Alloc>::value_type>{}(*key);
  }
};

#endif  // XYZ_INDIRECT_H