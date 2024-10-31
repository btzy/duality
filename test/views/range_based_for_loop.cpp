// This file is part of https://github.com/btzy/duality

#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <duality/viewifiers/contiguous.hpp>
#include <duality/views/reverse.hpp>
#include <duality/views/transform.hpp>
#include "../view_assert.hpp"

using namespace duality;

TEST_CASE("range_based_for_loop views", "[view range_based_for_loop]") {
    std::vector<int> vec{1, 2, 3, 4, 5};
    {
        std::vector<int> out;
        for (int x : vec | views::reverse) {
            out.push_back(x);
        }
        CHECK(out == std::vector<int>{5, 4, 3, 2, 1});
    }
    {
        std::vector<int> out;
        for (int x : vec | views::transform([](int x) { return x * 2; })) {
            out.push_back(x);
        }
        CHECK(out == std::vector<int>{2, 4, 6, 8, 10});
    }
}
