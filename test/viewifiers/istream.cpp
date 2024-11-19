// This file is part of https://github.com/btzy/duality

#include <sstream>
#include <string>

#include <duality/viewifiers/istream.hpp>
#include "../view_assert.hpp"

using namespace duality;

TEST_CASE("istream viewify", "[viewify istream]") {
    SECTION("int") {
        std::istringstream iss;
        view_assert_forward_singlepass(
            [&] {
                iss = std::istringstream("123 234 345 456 567");
                auto lvalue = viewify<int>(iss);
                static_assert(std::same_as<view_element_type_t<decltype(lvalue)>, int>);
                return lvalue;
            },
            {123, 234, 345, 456, 567});
        view_assert_forward_singlepass(
            [] {
                std::istringstream iss2("123 234 345 456 567");
                auto lvalue = viewify<int>(std::move(iss2));
                static_assert(std::same_as<view_element_type_t<decltype(lvalue)>, int>);
                return lvalue;
            },
            {123, 234, 345, 456, 567});
    }
    SECTION("string") {
        view_assert_forward_singlepass(
            [] {
                std::istringstream iss("123 234 345 456 567");
                auto lvalue = viewify<std::string>(std::move(iss));
                static_assert(std::same_as<view_element_type_t<decltype(lvalue)>, std::string>);
                return lvalue;
            },
            {"123", "234", "345", "456", "567"});
    }
}
