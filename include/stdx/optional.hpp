#pragma once

#include <stdx/type_traits.hpp>

#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

namespace stdx {
inline namespace v1 {
template <typename T, typename = void> struct tombstone_traits {
    static_assert(
        stdx::always_false_v<T>,
        "To use stdx::optional you must specialize stdx::tombstone_traits");
};

template <auto V> struct tombstone_value {
    constexpr auto operator()() const { return V; }
};

template <typename T, typename TS = tombstone_traits<T>> class optional {
    static_assert(not std::is_integral_v<T> or
                      not stdx::is_specialization_of_v<TS, tombstone_traits>,
                  "Don't define tombstone traits for plain integral types");
    constexpr static inline auto traits = TS{};
    [[no_unique_address]] T val{traits()};

  public:
    using value_type = T;

    constexpr optional() = default;
    constexpr explicit optional(std::nullopt_t) {}

    template <typename... Args>
    constexpr explicit optional(std::in_place_t, Args &&...args)
        : val{std::forward<Args>(args)...} {}

    template <
        typename U = T,
        typename = std::enable_if_t<
            std::is_constructible_v<T, U &&> and
            not std::is_same_v<stdx::remove_cvref_t<U>, std::in_place_t> and
            not std::is_same_v<stdx::remove_cvref_t<U>, optional>>>
    constexpr explicit optional(U &&u) : val{std::forward<U>(u)} {}

    constexpr auto operator=(std::nullopt_t) -> optional & {
        reset();
        return *this;
    }

    template <
        typename U = T,
        typename = std::enable_if_t<
            std::is_constructible_v<T, U> and std::is_assignable_v<T &, U> and
            not std::is_same_v<stdx::remove_cvref_t<U>, optional> and
            (std::is_scalar_v<T> or not std::is_same_v<std::decay_t<U>, T>)>>
    constexpr auto operator=(U &&u) -> optional & {
        val = std::forward<U>(u);
        return *this;
    }

    [[nodiscard]] constexpr auto has_value() const noexcept -> bool {
        return not(val == traits());
    }
    constexpr explicit operator bool() const noexcept { return has_value(); }

    [[nodiscard]] constexpr auto value() & -> value_type & { return val; }
    [[nodiscard]] constexpr auto value() const & -> value_type const & {
        return val;
    }
    [[nodiscard]] constexpr auto value() && -> value_type && {
        return std::move(val);
    }
    [[nodiscard]] constexpr auto value() const && -> value_type const && {
        return std::move(val);
    }

    [[nodiscard]] constexpr auto operator->() const -> value_type const * {
        return std::addressof(val);
    }
    [[nodiscard]] constexpr auto operator->() -> value_type * {
        return std::addressof(val);
    }

    [[nodiscard]] constexpr auto operator*() const & -> decltype(auto) {
        return value();
    }
    [[nodiscard]] constexpr auto operator*() & -> decltype(auto) {
        return value();
    }
    [[nodiscard]] constexpr auto operator*() const && -> decltype(auto) {
        return std::move(*this).value();
    }
    [[nodiscard]] constexpr auto operator*() && -> decltype(auto) {
        return std::move(*this).value();
    }

    template <typename U>
    [[nodiscard]] constexpr auto
    value_or(U &&default_value) const & -> value_type {
        return has_value() ? val : T{std::forward<U>(default_value)};
    }
    template <typename U>
    [[nodiscard]] constexpr auto value_or(U &&default_value) && -> value_type {
        return has_value() ? std::move(val) : T{std::forward<U>(default_value)};
    }

    template <typename... Args>
    constexpr auto emplace(Args &&...args) -> value_type & {
        val.~value_type();
        new (std::addressof(val)) value_type(std::forward<Args>(args)...);
        return value();
    }

    constexpr auto reset() {
        val.~value_type();
        new (std::addressof(val)) value_type(traits());
    }

    template <typename F> constexpr auto transform(F &&f) & {
        using func_t = stdx::remove_cvref_t<F>;
        using U = std::invoke_result_t<func_t, value_type &>;
        return *this ? optional<U>{std::forward<F>(f)(val)} : optional<U>{};
    }
    template <typename F> constexpr auto transform(F &&f) const & {
        using func_t = stdx::remove_cvref_t<F>;
        using U = std::invoke_result_t<func_t, value_type const &>;
        return *this ? optional<U>{std::forward<F>(f)(val)} : optional<U>{};
    }
    template <typename F> constexpr auto transform(F &&f) && {
        using func_t = stdx::remove_cvref_t<F>;
        using U = std::invoke_result_t<func_t, value_type &&>;
        return *this ? optional<U>{std::forward<F>(f)(std::move(val))}
                     : optional<U>{};
    }
    template <typename F> constexpr auto transform(F &&f) const && {
        using func_t = stdx::remove_cvref_t<F>;
        using U = std::invoke_result_t<func_t, value_type &&>;
        return *this ? optional<U>{std::forward<F>(f)(std::move(val))}
                     : optional<U>{};
    }

    template <typename F> constexpr auto or_else(F &&f) const & -> optional {
        return *this ? *this : std::forward<F>(f)();
    }
    template <typename F> constexpr auto or_else(F &&f) && -> optional {
        return *this ? std::move(*this) : std::forward<F>(f)();
    }

    template <typename F> constexpr auto and_then(F &&f) & {
        using func_t = stdx::remove_cvref_t<F>;
        using U = std::invoke_result_t<func_t, value_type &>;
        return *this ? std::forward<F>(f)(val) : U{};
    }
    template <typename F> constexpr auto and_then(F &&f) const & {
        using func_t = stdx::remove_cvref_t<F>;
        using U = std::invoke_result_t<func_t, value_type const &>;
        return *this ? std::forward<F>(f)(val) : U{};
    }
    template <typename F> constexpr auto and_then(F &&f) && {
        using func_t = stdx::remove_cvref_t<F>;
        using U = std::invoke_result_t<func_t, value_type &&>;
        return *this ? std::forward<F>(f)(std::move(val)) : U{};
    }
    template <typename F> constexpr auto and_then(F &&f) const && {
        using func_t = stdx::remove_cvref_t<F>;
        using U = std::invoke_result_t<func_t, value_type &&>;
        return *this ? std::forward<F>(f)(std::move(val)) : U{};
    }

  private:
    [[nodiscard]] friend constexpr auto operator==(optional const &lhs,
                                                   optional const &rhs)
        -> bool {
        return lhs.val == rhs.val;
    }

#if __cpp_impl_three_way_comparison < 201907L
    [[nodiscard]] friend constexpr auto operator!=(optional const &lhs,
                                                   optional const &rhs)
        -> bool {
        return not(lhs == rhs);
    }
#endif

    [[nodiscard]] friend constexpr auto operator<(optional const &lhs,
                                                  optional const &rhs) -> bool {
        return lhs.val < rhs.val and rhs.has_value();
    }
    [[nodiscard]] friend constexpr auto operator<=(optional const &lhs,
                                                   optional const &rhs)
        -> bool {
        return not(rhs < lhs);
    }
    [[nodiscard]] friend constexpr auto operator>(optional const &lhs,
                                                  optional const &rhs) -> bool {
        return rhs < lhs;
    }
    [[nodiscard]] friend constexpr auto operator>=(optional const &lhs,
                                                   optional const &rhs)
        -> bool {
        return not(lhs < rhs);
    }
};

template <typename T> optional(T) -> optional<T>;

template <typename F, typename... Ts,
          typename = std::enable_if_t<
              (... and stdx::is_specialization_of_v<stdx::remove_cvref_t<Ts>,
                                                    optional>)>>
constexpr auto transform(F &&f, Ts &&...ts) {
    using func_t = stdx::remove_cvref_t<F>;
    using U =
        std::invoke_result_t<func_t, decltype(std::forward<Ts>(ts).value())...>;
    if ((... and ts.has_value())) {
        return std::forward<F>(f)(std::forward<Ts>(ts).value()...);
    }
    return U{};
}
} // namespace v1
} // namespace stdx
