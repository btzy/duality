// This file is part of https://github.com/btzy/duality

#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <ranges>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

#include <duality/operations/to.hpp>
#include <duality/viewifiers/contiguous.hpp>
#include <duality/views/filter.hpp>
#include <duality/views/transform.hpp>

using namespace duality;

template <std::ranges::input_range R, typename T>
    requires std::equality_comparable_with<std::ranges::range_value_t<R>, T>
void range_assert(R&& r, const std::vector<T>& expected) {
    CHECK_THAT(r, Catch::Matchers::RangeEquals(expected));
}

template <std::ranges::input_range R, typename T>
    requires std::equality_comparable_with<std::ranges::range_value_t<R>, T>
void range_assert_unordered(R&& r, const std::vector<T>& expected) {
    CHECK_THAT(r, Catch::Matchers::UnorderedRangeEquals(expected));
}

TEST_CASE("to operations", "[operations to]") {
    std::vector<int> vec{1, 2, 3, 4, 5};
    auto v = viewify(vec);

    // regular containers

    range_assert(
        v | views::transform([](int x) { return x * 2; }) | operations::to<std::vector<int>>(),
        std::vector<int>{2, 4, 6, 8, 10});
    range_assert(
        v | views::filter([](int x) { return x % 2 == 0; }) | operations::to<std::vector<int>>(),
        std::vector<int>{2, 4});
    range_assert(
        v | views::transform([](int x) { return x * 2; }) | operations::to<std::deque<int>>(),
        std::vector<int>{2, 4, 6, 8, 10});
    range_assert(
        v | views::filter([](int x) { return x % 2 == 0; }) | operations::to<std::deque<int>>(),
        std::vector<int>{2, 4});
    range_assert(
        v | views::transform([](int x) { return x * 2; }) | operations::to<std::list<int>>(),
        std::vector<int>{2, 4, 6, 8, 10});
    range_assert(
        v | views::filter([](int x) { return x % 2 == 0; }) | operations::to<std::list<int>>(),
        std::vector<int>{2, 4});
    range_assert(v | views::transform([](int x) { return x * 2; }) |
                     operations::to<std::forward_list<int>>(),
                 std::vector<int>{2, 4, 6, 8, 10});
    range_assert(v | views::filter([](int x) { return x % 2 == 0; }) |
                     operations::to<std::forward_list<int>>(),
                 std::vector<int>{2, 4});
    range_assert(
        v | views::transform([](int x) { return x * 2; }) | operations::to<std::set<int>>(),
        std::vector<int>{2, 4, 6, 8, 10});
    range_assert(
        v | views::filter([](int x) { return x % 2 == 0; }) | operations::to<std::set<int>>(),
        std::vector<int>{2, 4});
    range_assert_unordered(v | views::transform([](int x) { return x * 2; }) |
                               operations::to<std::unordered_set<int>>(),
                           std::vector<int>{2, 4, 6, 8, 10});
    range_assert_unordered(v | views::filter([](int x) { return x % 2 == 0; }) |
                               operations::to<std::unordered_set<int>>(),
                           std::vector<int>{2, 4});

    // associative containers

    range_assert(v | views::transform([](int x) {
                     return std::pair{x, x * 2};
                 }) | operations::to<std::map<int, int>>(),
                 std::vector<std::pair<const int, int>>{{1, 2}, {2, 4}, {3, 6}, {4, 8}, {5, 10}});
    range_assert(v | views::transform([](int x) {
                     return std::pair{x, x * 2};
                 }) | views::filter([](std::pair<int, int> x) { return x.first % 2 == 0; }) |
                     operations::to<std::map<int, int>>(),
                 std::vector<std::pair<const int, int>>{{2, 4}, {4, 8}});
    range_assert_unordered(
        v | views::transform([](int x) {
            return std::pair{x, x * 2};
        }) | operations::to<std::unordered_map<int, int>>(),
        std::vector<std::pair<const int, int>>{{1, 2}, {2, 4}, {3, 6}, {4, 8}, {5, 10}});
    range_assert_unordered(v | views::transform([](int x) {
                               return std::pair{x, x * 2};
                           }) | views::filter([](std::pair<int, int> x) {
                               return x.first % 2 == 0;
                           }) | operations::to<std::unordered_map<int, int>>(),
                           std::vector<std::pair<const int, int>>{{2, 4}, {4, 8}});
}
