// This file is part of https://github.com/btzy/duality
#pragma once

#include <type_traits>
#include <utility>
#include <vector>

#include <duality/core_view.hpp>

#include <catch2/catch_test_macros.hpp>

template <typename ViewMaker>
inline void view_assert_forward_singlepass(
    ViewMaker&& view_maker,
    const std::vector<
        std::remove_cvref_t<duality::view_element_type_t<std::invoke_result_t<ViewMaker>>>>&
        expected) {
    // forward check
    {
        auto v = view_maker();
        auto it = expected.begin();
        auto fit = v.forward_iter();
        const auto rit = v.backward_iter();
        while (auto opt = fit.next(rit)) {
            REQUIRE(it != expected.end());
            CHECK(*opt == *it++);
        }
        CHECK(it == expected.end());
    }

    {
        auto v = view_maker();
        auto it = expected.begin();
        auto fit = v.forward_iter();
        const auto rit = v.backward_iter();
        while (fit.skip(rit)) {
            REQUIRE(it != expected.end());
            ++it;
        }
        CHECK(it == expected.end());
    }

    {
        auto v = view_maker();
        auto fit = v.forward_iter();
        for (auto&& x : expected) {
            REQUIRE(fit.next() == x);
        }
        CHECK_FALSE(fit.next(v.backward_iter()));
    }

    {
        auto v = view_maker();
        auto fit = v.forward_iter();
        for (auto&& x : expected) {
            (void)x;
            fit.skip();
        }
        CHECK_FALSE(fit.skip(v.backward_iter()));
    }
}

template <typename V,
          typename E = std::vector<std::remove_cvref_t<duality::view_element_type_t<V>>>,
          typename Equals = std::equal_to<>>
inline void view_assert_forward(V&& v, const E& expected, Equals&& equals = {}) {
    // forward check
    {
        auto it = expected.begin();
        auto fit = v.forward_iter();
        const auto rit = v.backward_iter();
        while (auto opt = fit.next(rit)) {
            REQUIRE(it != expected.end());
            CHECK(equals(*opt, *it++));
        }
        CHECK(it == expected.end());
    }

    {
        auto it = expected.begin();
        auto fit = v.forward_iter();
        const auto rit = v.backward_iter();
        while (fit.skip(rit)) {
            REQUIRE(it != expected.end());
            ++it;
        }
        CHECK(it == expected.end());
    }

    {
        auto fit = v.forward_iter();
        for (auto&& x : expected) {
            REQUIRE(equals(fit.next(), x));
        }
        CHECK_FALSE(fit.next(v.backward_iter()));
    }

    {
        auto fit = v.forward_iter();
        for (auto&& x : expected) {
            (void)x;
            fit.skip();
        }
        CHECK_FALSE(fit.skip(v.backward_iter()));
    }
}

template <typename V,
          typename E = std::vector<std::remove_cvref_t<duality::view_element_type_t<V>>>,
          typename Equals = std::equal_to<>>
inline void view_assert_forward_infinite(V&& v, const E& expected, Equals&& equals = {}) {
    // forward check
    {
        auto fit = v.forward_iter();
        const auto rit = v.backward_iter();
        for (auto&& x : expected) {
            auto opt = fit.next(rit);
            REQUIRE(opt);
            CHECK(equals(*opt, x));
        }
        CHECK(fit.next(rit));
    }

    {
        auto fit = v.forward_iter();
        const auto rit = v.backward_iter();
        for (auto&& x : expected) {
            (void)x;
            REQUIRE(fit.skip(rit));
        }
        CHECK(fit.next(rit));
    }

    {
        auto fit = v.forward_iter();
        for (auto&& x : expected) {
            REQUIRE(equals(fit.next(), x));
        }
        CHECK(fit.next(v.backward_iter()));
    }

    {
        auto fit = v.forward_iter();
        for (auto&& x : expected) {
            (void)x;
            fit.skip();
        }
        CHECK(fit.skip(v.backward_iter()));
    }
}

template <typename V,
          typename E = std::vector<std::remove_cvref_t<duality::view_element_type_t<V>>>,
          typename Equals = std::equal_to<>>
inline void view_assert_backward(V&& v, const E& expected, Equals&& equals = {}) {
    // backward check
    {
        auto it = expected.rbegin();
        const auto fit = v.forward_iter();
        auto rit = v.backward_iter();
        while (auto opt = rit.next(fit)) {
            REQUIRE(it != expected.rend());
            CHECK(equals(*opt, *it++));
        }
        CHECK(it == expected.rend());
    }

    {
        auto it = expected.rbegin();
        const auto fit = v.forward_iter();
        auto rit = v.backward_iter();
        while (rit.skip(fit)) {
            REQUIRE(it != expected.rend());
            ++it;
        }
        CHECK(it == expected.rend());
    }

    {
        auto fit = v.backward_iter();
        for (auto it = expected.rbegin(); it != expected.rend(); ++it) {
            REQUIRE(equals(fit.next(), *it));
        }
        CHECK_FALSE(fit.next(v.forward_iter()));
    }

    {
        auto fit = v.backward_iter();
        for (auto&& x : expected) {
            (void)x;
            fit.skip();
        }
        CHECK_FALSE(fit.skip(v.forward_iter()));
    }
}

template <typename V,
          typename E = std::vector<std::remove_cvref_t<duality::view_element_type_t<V>>>,
          typename Equals = std::equal_to<>>
inline void view_assert_backward_infinite(V&& v, const E& expected, Equals&& equals = {}) {
    // TODO
    (void)v;
    (void)expected;
    (void)equals;
}

template <typename V,
          typename E = std::vector<std::remove_cvref_t<duality::view_element_type_t<V>>>,
          typename Equals = std::equal_to<>>
inline void view_assert_bidirectional(V&& v, const E& expected, Equals&& equals = {}) {
    view_assert_forward(v, expected, equals);
    view_assert_backward(v, expected, equals);

    // bidirectional check
    auto it_begin = expected.begin();
    auto it_end = expected.end();
    auto fit = v.forward_iter();
    auto rit = v.backward_iter();
    bool from_back = false;
    while (true) {
        if (!from_back) {
            if (auto opt = fit.next(std::as_const(rit))) {
                REQUIRE(it_begin != it_end);
                CHECK(equals(*opt, *it_begin++));
            } else {
                break;
            }
        } else {
            if (auto opt = rit.next(std::as_const(fit))) {
                REQUIRE(it_begin != it_end);
                CHECK(equals(*opt, *--it_end));
            } else {
                break;
            }
        }
    }
    CHECK(it_begin == it_end);
}

template <typename DualityIter,
          typename DualityEndIter,
          typename StdIter,
          typename Equals = std::equal_to<>>
inline void check_inverted(DualityIter fit,
                           const DualityEndIter rit,
                           StdIter curr,
                           StdIter from,
                           Equals&& equals = {}) {
    for (size_t i = 0; i != 2; ++i) {
        if (auto opt = fit.next(rit)) {
            REQUIRE(curr != from);
            CHECK(equals(*--curr, *opt));
        } else {
            CHECK(curr == from);
            break;
        }
    }
}

template <typename V,
          typename E = std::vector<std::remove_cvref_t<duality::view_element_type_t<V>>>,
          typename Equals = std::equal_to<>>
inline void view_assert_multipass_bidirectional(V&& v, const E& expected, Equals&& equals = {}) {
    view_assert_bidirectional(v, expected, equals);

    // invertibility check (forward)
    {
        auto it = expected.begin();
        auto fit = v.forward_iter();
        const auto rit = v.backward_iter();
        check_inverted(fit.invert(), fit, it, expected.begin(), equals);
        while (auto opt = fit.next(rit)) {
            REQUIRE(it != expected.end());
            CHECK(equals(*opt, *it++));
            check_inverted(fit.invert(), v.forward_iter(), it, expected.begin(), equals);
        }
        CHECK(it == expected.end());
    }

    // invertibility check (backward)
    {
        auto it = expected.rbegin();
        const auto fit = v.forward_iter();
        auto rit = v.backward_iter();
        check_inverted(rit.invert(), rit, it, expected.rbegin(), equals);
        while (auto opt = rit.next(fit)) {
            REQUIRE(it != expected.rend());
            CHECK(equals(*opt, *it++));
            check_inverted(rit.invert(), v.backward_iter(), it, expected.rbegin(), equals);
        }
        CHECK(it == expected.rend());
    }
}

template <typename V,
          typename E = std::vector<std::remove_cvref_t<duality::view_element_type_t<V>>>,
          typename Equals = std::equal_to<>>
inline void view_assert_multipass_forward(V&& v, const E& expected, Equals&& equals = {}) {
    view_assert_forward(v, expected, equals);

    // TODO check multipass property.
}

template <typename V,
          typename E = std::vector<std::remove_cvref_t<duality::view_element_type_t<V>>>>
inline void view_assert_emptyness(V&& v, const E& expected) {
    // emptyness check
    CHECK(v.empty() == expected.empty());
}

template <typename V,
          typename E = std::vector<std::remove_cvref_t<duality::view_element_type_t<V>>>>
inline void view_assert_sized(V&& v, const E& expected) {
    view_assert_emptyness(v, expected);

    // size check
    CHECK(v.size() == expected.size());
}

/// Checks if `v` as an infinite_random_access_view starts with the elements in `expected`.  The
/// view of course has more elements than `expected`, so we only check that the first
/// `expected.size()` elements match expect more elements to be consumable.
template <typename V,
          typename E = std::vector<std::remove_cvref_t<duality::view_element_type_t<V>>>,
          typename Equals = std::equal_to<>>
inline void view_assert_infinite_multipass_forward(V&& v, const E& expected, Equals&& equals = {}) {
    view_assert_forward_infinite(v, expected, equals);

    // invertibility check (forward)
    {
        auto fit = v.forward_iter();
        const auto rit = v.backward_iter();
        for (auto it = expected.begin(); it != expected.end(); ++it) {
            check_inverted(fit.invert(), v.forward_iter(), it, expected.begin(), equals);
            auto opt = fit.next(rit);
            REQUIRE(opt);
            CHECK(equals(*opt, *it));
        }
        check_inverted(fit.invert(), v.forward_iter(), expected.end(), expected.begin(), equals);
    }
}

template <typename V,
          typename E = std::vector<std::remove_cvref_t<duality::view_element_type_t<V>>>,
          typename Equals = std::equal_to<>>
inline void view_assert_infinite_multipass_backward(V&& v,
                                                    const E& expected,
                                                    Equals&& equals = {}) {
    view_assert_backward_infinite(v, expected, equals);

    // invertibility check (backward)
    // TODO
}

/// Checks if `v` as an infinite_random_access_view starts with the elements in `expected`.  The
/// view of course has more elements than `expected`, so we only check that the first
/// `expected.size()` elements match expect more elements to be consumable.
template <typename V,
          typename E = std::vector<std::remove_cvref_t<duality::view_element_type_t<V>>>,
          typename Equals = std::equal_to<>>
inline void view_assert_infinite_random_access_forward(V&& v,
                                                       const E& expected,
                                                       Equals&& equals = {}) {
    view_assert_infinite_multipass_forward(v, expected, equals);

    // forward skip(n, it)
    {
        auto fit = v.forward_iter();
        const auto rit = v.backward_iter();
        const size_t half = expected.size() / 2;
        CHECK(fit.skip(0, rit) == 0);
        CHECK(fit.skip(half, rit) == half);
        CHECK(fit.skip(expected.size() - half, rit) == expected.size() - half);
        CHECK(fit.skip(rit));
    }

    // forward skip(n)
    {
        auto fit = v.forward_iter();
        const auto rit = v.backward_iter();
        const size_t half = expected.size() / 2;
        fit.skip(0);
        fit.skip(half);
        fit.skip(expected.size() - half);
        CHECK(fit.skip(rit));
    }
}
template <typename V,
          typename E = std::vector<std::remove_cvref_t<duality::view_element_type_t<V>>>,
          typename Equals = std::equal_to<>>
inline void view_assert_infinite_random_access_backward(V&& v,
                                                        const E& expected,
                                                        Equals&& equals = {}) {
    view_assert_infinite_multipass_backward(v, expected, equals);

    // backward skip(n, it)
    {
        auto rit = v.backward_iter();
        const auto fit = v.forward_iter();
        const size_t half = expected.size() / 2;
        CHECK(rit.skip(0, fit) == 0);
        CHECK(rit.skip(half, fit) == half);
        CHECK(rit.skip(expected.size() - half, fit) == expected.size() - half);
        CHECK(rit.skip(fit));
    }

    // backward skip(n)
    {
        auto rit = v.backward_iter();
        const auto fit = v.forward_iter();
        const size_t half = expected.size() / 2;
        rit.skip(0);
        rit.skip(half);
        rit.skip(expected.size() - half);
        CHECK(rit.skip(fit));
    }
}

template <typename V,
          typename E = std::vector<std::remove_cvref_t<duality::view_element_type_t<V>>>,
          typename Equals = std::equal_to<>>
inline void view_assert_random_access_bidirectional(V&& v,
                                                    const E& expected,
                                                    Equals&& equals = {}) {
    view_assert_multipass_bidirectional(v, expected, equals);
    view_assert_sized(v, expected);

    // // indexing check
    // for (size_t i = 0; i != expected.size(); ++i) {
    //     CHECK(v[i] == expected[i]);
    // }

    // forward skip(n, it)
    {
        auto fit = v.forward_iter();
        const auto rit = v.backward_iter();
        const size_t half = expected.size() / 2;
        CHECK(fit.skip(0, rit) == 0);
        CHECK(fit.skip(half, rit) == half);
        CHECK(fit.skip(expected.size() - half, rit) == expected.size() - half);
        CHECK_FALSE(fit.skip(rit));
    }

    // forward skip(n)
    {
        auto fit = v.forward_iter();
        const auto rit = v.backward_iter();
        const size_t half = expected.size() / 2;
        fit.skip(0);
        fit.skip(half);
        fit.skip(expected.size() - half);
        CHECK_FALSE(fit.skip(rit));
    }

    // backward skip(n, it)
    {
        const auto fit = v.forward_iter();
        auto rit = v.backward_iter();
        const size_t half = expected.size() / 2;
        CHECK(rit.skip(0, fit) == 0);
        CHECK(rit.skip(half, fit) == half);
        CHECK(rit.skip(expected.size() - half, fit) == expected.size() - half);
        CHECK_FALSE(rit.skip(fit));
    }

    // backward skip(n)
    {
        const auto fit = v.forward_iter();
        auto rit = v.backward_iter();
        const size_t half = expected.size() / 2;
        rit.skip(0);
        rit.skip(half);
        rit.skip(expected.size() - half);
        CHECK_FALSE(rit.skip(fit));
    }
}
