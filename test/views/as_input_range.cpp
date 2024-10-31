// This file is part of https://github.com/btzy/duality

#include <ranges>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

#include <duality/viewifiers/contiguous.hpp>
#include <duality/views/as_input_range.hpp>
#include <duality/views/filter.hpp>
#include <duality/views/transform.hpp>

using namespace duality;

template <std::ranges::input_range R>
void range_assert(R&& r, const std::vector<std::ranges::range_value_t<R>>& expected) {
    CHECK_THAT(r, Catch::Matchers::RangeEquals(expected));
}

TEST_CASE("as_input_range views", "[view as_input_range]") {
    std::vector<int> vec{1, 2, 3, 4, 5};
    auto v = viewify(vec);
    range_assert(v | views::as_input_range, std::vector<int>{1, 2, 3, 4, 5});
    range_assert(v | views::transform([](int x) { return x * 2; }) | views::as_input_range,
                 std::vector<int>{2, 4, 6, 8, 10});
    range_assert(v | views::filter([](int x) { return x % 2 == 0; }) | views::as_input_range,
                 std::vector<int>{2, 4});
}
