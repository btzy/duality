// This file is part of https://github.com/btzy/duality
#pragma once

#include <concepts>

// A range is a view made from a pair of matching iterators (i.e. a forward iterator and a backward
// iterator from the same container, that have not yet crossed each other).

namespace duality {

template <typename ForwardIter, typename BackwardIter>
    requires sentinel_for<BackwardIter, ForwardIter> || sentinel_for<ForwardIter, BackwardIter>
class range {
   private:
    [[no_unique_address]] ForwardIter forward_it_;
    [[no_unique_address]] BackwardIter backward_it_;

   public:
    template <typename F, typename B>
    constexpr range(F&& forward_it,
                    B&& backward_it) noexcept(std::is_nothrow_constructible_v<ForwardIter, F> &&
                                              std::is_nothrow_constructible_v<BackwardIter, B>)
        : forward_it_(forward_it), backward_it_(backward_it) {}
    constexpr ForwardIter forward_iter() const
        noexcept(std::is_nothrow_copy_constructible_v<ForwardIter>) {
        return forward_it_;
    }
    constexpr BackwardIter backward_iter() const
        noexcept(std::is_nothrow_copy_constructible_v<BackwardIter>) {
        return backward_it_;
    }
};

template <typename ForwardIter, typename BackwardIter>
range(ForwardIter&&, BackwardIter&&)
    -> range<std::remove_reference_t<ForwardIter>, std::remove_reference_t<BackwardIter>>;

template <typename ForwardIter, typename BackwardIter>
    requires sentinel_for<BackwardIter, ForwardIter> || sentinel_for<ForwardIter, BackwardIter>
class move_range {
   private:
    [[no_unique_address]] ForwardIter forward_it_;
    [[no_unique_address]] BackwardIter backward_it_;

   public:
    template <typename F, typename B>
    constexpr move_range(F&& forward_it, B&& backward_it) noexcept(
        std::is_nothrow_constructible_v<ForwardIter, F> &&
        std::is_nothrow_constructible_v<BackwardIter, B>)
        : forward_it_(forward_it), backward_it_(backward_it) {}
    constexpr ForwardIter forward_iter() noexcept(
        std::is_nothrow_move_constructible_v<ForwardIter>) {
        return std::move(forward_it_);
    }
    constexpr BackwardIter backward_iter() noexcept(
        std::is_nothrow_move_constructible_v<BackwardIter>) {
        return std::move(backward_it_);
    }
};

template <typename ForwardIter, typename BackwardIter>
move_range(ForwardIter&&, BackwardIter&&)
    -> move_range<std::remove_reference_t<ForwardIter>, std::remove_reference_t<BackwardIter>>;

}  // namespace duality
