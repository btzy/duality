// This file is part of https://github.com/btzy/duality
#pragma once

#include <concepts>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

#include <duality/core_stage_view.hpp>

// Note: Unnamed unions currently don't respect [[no_unique_address]], but it isn't clear if it is
// meant to work.

namespace duality {

namespace impl {
template <typename V>
concept forward_joinable_view = forward_view<V> && forward_view<view_element_type_t<V>>;
template <typename V>
concept backward_joinable_view = backward_view<V> && backward_view<view_element_type_t<V>>;
template <typename V>
concept bidirectional_joinable_view = forward_joinable_view<V> && backward_joinable_view<V>;
}  // namespace impl

// template <joinable_view V>
// class join_stage_view;

namespace impl {
struct join_stage_view_empty_tag {};

// the sentinel for join_forward_iterator
template <typename I>
class join_forward_sentinel {
   private:
    template <forward_joinable_view>
    friend class join_forward_view;
    template <iterator>
    friend class join_forward_iterator;
    [[no_unique_address]] I i_;

    template <typename I2>
    constexpr join_forward_sentinel(wrapping_construct_t,
                                    I2&& i) noexcept(std::is_nothrow_constructible_v<I, I2>)
        : i_(std::forward<I2>(i)) {}
};

template <iterator I>
struct join_forward_view_stage {
    // current value of the outer iterator
    [[no_unique_address]] I i;
    // controlled by forward iterator
    union cache {
        [[no_unique_address]] join_stage_view_empty_tag empty;
        [[no_unique_address]] iterator_element_type_t<I> filled;
        constexpr cache() noexcept : empty() {}
        // join_forward_view_stage may only be copied or moved when there are no live iterators, so
        // we can assume it is empty
        constexpr cache(const cache&) noexcept : empty() {}
        constexpr cache& operator=(const cache&) noexcept {}
    } cache;
    // controlled by forward iterator
    union cache_end {
        [[no_unique_address]] join_stage_view_empty_tag empty;
        [[no_unique_address]] decltype(std::declval<iterator_element_type_t<I>>()
                                           .backward_iter()) filled;
        constexpr cache_end() noexcept : empty() {}
        // join_forward_view_stage may only be copied or moved when there are no live iterators, so
        // we can assume it is empty
        constexpr cache_end(const cache_end&) noexcept : empty() {}
        constexpr cache_end& operator=(const cache_end&) noexcept {}
    } cache_end;
};

template <forward_joinable_view V>
class join_forward_view;

template <iterator I>
class join_forward_iterator {
   private:
    template <forward_joinable_view>
    friend class join_forward_view;

    // non-null pointer to staging area
    join_forward_view_stage<I>* stage_;
    // whether cache_, cache_end_, and cache_it_ contain values
    bool started_;

    using InnerI = decltype(std::declval<iterator_element_type_t<I>>().forward_iter());
    using InnerS = decltype(std::declval<iterator_element_type_t<I>>().backward_iter());
    // populated only if started_
    union {
        [[no_unique_address]] join_stage_view_empty_tag empty_;
        [[no_unique_address]] InnerI cache_it_;
    };

    using element_type = iterator_element_type_t<InnerI>;

    constexpr join_forward_iterator(wrapping_construct_t,
                                    join_forward_view_stage<I>& stage) noexcept
        : stage_(&stage), started_(false) {}

    constexpr auto& inner_iter() { return *std::launder(&cache_it_); }
    constexpr auto& inner_end() { return *std::launder(&stage_->cache_end.filled); }
    constexpr auto& inner_cache() { return *std::launder(&stage_->cache.filled); }
    constexpr auto& inner_iter_unlaundered() { return cache_it_; }
    constexpr auto& inner_end_unlaundered() { return stage_->cache_end.filled; }
    constexpr auto& inner_cache_unlaundered() { return stage_->cache.filled; }
    constexpr auto& outer_iter() { return stage_->i; }

   public:
    constexpr join_forward_iterator() noexcept : stage_(nullptr), started_(false) {};
    constexpr join_forward_iterator(join_forward_iterator&& other) noexcept
        : stage_(std::exchange(other.stage_, nullptr)),
          started_(std::exchange(other.started_, false)) {
        if (started_) {
            auto& other_inner_iter = other.inner_iter();
            std::construct_at(&inner_iter_unlaundered(), std::move(other_inner_iter));
            std::destroy_at(&other_inner_iter);
        }
    };
    constexpr join_forward_iterator& operator=(join_forward_iterator&& other) noexcept {
        if (started_) {
            std::destroy_at(&inner_iter());
            std::destroy_at(&inner_end());
            std::destroy_at(&inner_cache());
        }
        stage_ = std::exchange(other.stage_, nullptr);
        started_ = std::exchange(other.started_, false);
        if (started_) {
            auto& other_inner_iter = other.inner_iter();
            std::construct_at(&inner_iter_unlaundered(), std::move(other_inner_iter));
            std::destroy_at(&other_inner_iter);
        }
    };
    constexpr ~join_forward_iterator() {
        if (started_) {
            std::destroy_at(&inner_iter());
            std::destroy_at(&inner_end());
            std::destroy_at(&inner_cache());
        }
    }

    using index_type = no_index_type_t;
    constexpr element_type next() {
#if !defined(__cpp_constexpr) || __cpp_constexpr < 202110L
        decltype(&inner_iter()) in_iter;
        decltype(&inner_end()) in_end;
        if (started_) {
            in_iter = &inner_iter();
            in_end = &inner_end();
        }
        while (true) {
            if (started_) {
                if (auto opt = in_iter->next(*in_end)) {
                    return *std::move(opt);
                }
                std::destroy_at(in_iter);
                std::destroy_at(in_end);
                std::destroy_at(&inner_cache());
            } else {
                started_ = true;
            }
            // seems like we can't have guaranteed copy elision with std::construct_at
            try {
                auto& in_cache =
                    *std::construct_at(&inner_cache_unlaundered(), outer_iter().next());
                try {
                    in_end = std::construct_at(&inner_end_unlaundered(), in_cache.backward_iter());
                    try {
                        in_iter =
                            std::construct_at(&inner_iter_unlaundered(), in_cache.forward_iter());
                    } catch (...) {
                        std::destroy_at(in_end);
                        throw;
                    }
                } catch (...) {
                    std::destroy_at(&in_cache);
                    throw;
                }
            } catch (...) {
                started_ = false;
                throw;
            }
        }
#else
        decltype(&inner_iter()) in_iter;
        decltype(&inner_end()) in_end;
        if (!started_) {
            started_ = true;
            goto unstarted_jump;
        }
        in_iter = &inner_iter();
        in_end = &inner_end();
        while (true) {
            if (auto opt = in_iter->next(*in_end)) {
                return *std::move(opt);
            }
            std::destroy_at(in_iter);
            std::destroy_at(in_end);
            std::destroy_at(&inner_cache());
        unstarted_jump:
            // seems like we can't have guaranteed copy elision with std::construct_at
            try {
                auto& in_cache =
                    *std::construct_at(&inner_cache_unlaundered(), outer_iter().next());
                try {
                    in_end = std::construct_at(&inner_end_unlaundered(), in_cache.backward_iter());
                    try {
                        in_iter =
                            std::construct_at(&inner_iter_unlaundered(), in_cache.forward_iter());
                    } catch (...) {
                        std::destroy_at(in_end);
                        throw;
                    }
                } catch (...) {
                    std::destroy_at(&in_cache);
                    throw;
                }
            } catch (...) {
                started_ = false;
                throw;
            }
        }
#endif
    }
    template <sentinel_for<I> S>
    constexpr optional<element_type> next(const join_forward_sentinel<S>& s) {
#if !defined(__cpp_constexpr) || __cpp_constexpr < 202110L
        decltype(&inner_iter()) in_iter;
        decltype(&inner_end()) in_end;
        if (started_) {
            in_iter = &inner_iter();
            in_end = &inner_end();
        }
        while (true) {
            if (started_) {
                if (auto opt = in_iter->next(*in_end)) {
                    return opt;
                }
                std::destroy_at(in_iter);
                std::destroy_at(in_end);
                std::destroy_at(&inner_cache());
            } else {
                started_ = true;
            }
            if (auto opt = outer_iter().next(s.i_)) {
                // seems like we can't have guaranteed copy elision with std::construct_at
                try {
                    auto& in_cache = *std::construct_at(&inner_cache_unlaundered(), *opt);
                    try {
                        in_end =
                            std::construct_at(&inner_end_unlaundered(), in_cache.backward_iter());
                        try {
                            in_iter = std::construct_at(&inner_iter_unlaundered(),
                                                        in_cache.forward_iter());
                        } catch (...) {
                            std::destroy_at(in_end);
                            throw;
                        }
                    } catch (...) {
                        std::destroy_at(&in_cache);
                        throw;
                    }
                } catch (...) {
                    started_ = false;
                    throw;
                }
            } else {
                started_ = false;
                return nullopt;
            }
        }
#else
        decltype(&inner_iter()) in_iter;
        decltype(&inner_end()) in_end;
        if (!started_) {
            started_ = true;
            goto unstarted_jump;
        }
        in_iter = &inner_iter();
        in_end = &inner_end();
        while (true) {
            if (auto opt = in_iter->next(*in_end)) {
                return opt;
            }
            std::destroy_at(in_iter);
            std::destroy_at(in_end);
            std::destroy_at(&inner_cache());
        unstarted_jump:
            if (auto opt = outer_iter().next(s.i_)) {
                // seems like we can't have guaranteed copy elision with std::construct_at
                try {
                    auto& in_cache = *std::construct_at(&inner_cache_unlaundered(), *opt);
                    try {
                        in_end =
                            std::construct_at(&inner_end_unlaundered(), in_cache.backward_iter());
                        try {
                            in_iter = std::construct_at(&inner_iter_unlaundered(),
                                                        in_cache.forward_iter());
                        } catch (...) {
                            std::destroy_at(in_end);
                            throw;
                        }
                    } catch (...) {
                        std::destroy_at(&in_cache);
                        throw;
                    }
                } catch (...) {
                    started_ = false;
                    throw;
                }
            } else {
                started_ = false;
                return nullopt;
            }
        }
#endif
    }
    constexpr void skip() {
#if !defined(__cpp_constexpr) || __cpp_constexpr < 202110L
        decltype(&inner_iter()) in_iter;
        decltype(&inner_end()) in_end;
        if (started_) {
            in_iter = &inner_iter();
            in_end = &inner_end();
        }
        while (true) {
            if (started_) {
                if (in_iter->skip(*in_end)) {
                    return;
                }
                std::destroy_at(in_iter);
                std::destroy_at(in_end);
                std::destroy_at(&inner_cache());
            } else {
                started_ = true;
            }
            // seems like we can't have guaranteed copy elision with std::construct_at
            try {
                auto& in_cache =
                    *std::construct_at(&inner_cache_unlaundered(), outer_iter().next());
                try {
                    in_end = std::construct_at(&inner_end_unlaundered(), in_cache.backward_iter());
                    try {
                        in_iter =
                            std::construct_at(&inner_iter_unlaundered(), in_cache.forward_iter());
                    } catch (...) {
                        std::destroy_at(in_end);
                        throw;
                    }
                } catch (...) {
                    std::destroy_at(&in_cache);
                    throw;
                }
            } catch (...) {
                started_ = false;
                throw;
            }
        }
#else
        decltype(&inner_iter()) in_iter;
        decltype(&inner_end()) in_end;
        if (!started_) {
            started_ = true;
            goto unstarted_jump;
        }
        in_iter = &inner_iter();
        in_end = &inner_end();
        while (true) {
            if (in_iter->skip(*in_end)) {
                return;
            }
            std::destroy_at(in_iter);
            std::destroy_at(in_end);
            std::destroy_at(&inner_cache());
        unstarted_jump:
            // seems like we can't have guaranteed copy elision with std::construct_at
            try {
                auto& in_cache =
                    *std::construct_at(&inner_cache_unlaundered(), outer_iter().next());
                try {
                    in_end = std::construct_at(&inner_end_unlaundered(), in_cache.backward_iter());
                    try {
                        in_iter =
                            std::construct_at(&inner_iter_unlaundered(), in_cache.forward_iter());
                    } catch (...) {
                        std::destroy_at(in_end);
                        throw;
                    }
                } catch (...) {
                    std::destroy_at(&in_cache);
                    throw;
                }
            } catch (...) {
                started_ = false;
                throw;
            }
        }
#endif
    }
    template <sentinel_for<I> S>
    constexpr bool skip(const join_forward_sentinel<S>& s) {
#if !defined(__cpp_constexpr) || __cpp_constexpr < 202110L
        decltype(&inner_iter()) in_iter;
        decltype(&inner_end()) in_end;
        if (started_) {
            in_iter = &inner_iter();
            in_end = &inner_end();
        }
        while (true) {
            if (started_) {
                if (in_iter->skip(*in_end)) {
                    return true;
                }
                std::destroy_at(in_iter);
                std::destroy_at(in_end);
                std::destroy_at(&inner_cache());
            } else {
                started_ = true;
            }
            if (auto opt = outer_iter().next(s.i_)) {
                // seems like we can't have guaranteed copy elision with std::construct_at
                try {
                    auto& in_cache = *std::construct_at(&inner_cache_unlaundered(), *opt);
                    try {
                        in_end =
                            std::construct_at(&inner_end_unlaundered(), in_cache.backward_iter());
                        try {
                            in_iter = std::construct_at(&inner_iter_unlaundered(),
                                                        in_cache.forward_iter());
                        } catch (...) {
                            std::destroy_at(in_end);
                            throw;
                        }
                    } catch (...) {
                        std::destroy_at(&in_cache);
                        throw;
                    }
                } catch (...) {
                    started_ = false;
                    throw;
                }
            } else {
                started_ = false;
                return false;
            }
        }
#else
        decltype(&inner_iter()) in_iter;
        decltype(&inner_end()) in_end;
        if (!started_) {
            started_ = true;
            goto unstarted_jump;
        }
        in_iter = &inner_iter();
        in_end = &inner_end();
        while (true) {
            if (in_iter->skip(*in_end)) {
                return true;
            }
            std::destroy_at(in_iter);
            std::destroy_at(in_end);
            std::destroy_at(&inner_cache());
        unstarted_jump:
            if (auto opt = outer_iter().next(s.i_)) {
                // seems like we can't have guaranteed copy elision with std::construct_at
                try {
                    auto& in_cache = *std::construct_at(&inner_cache_unlaundered(), *opt);
                    try {
                        in_end =
                            std::construct_at(&inner_end_unlaundered(), in_cache.backward_iter());
                        try {
                            in_iter = std::construct_at(&inner_iter_unlaundered(),
                                                        in_cache.forward_iter());
                        } catch (...) {
                            std::destroy_at(in_end);
                            throw;
                        }
                    } catch (...) {
                        std::destroy_at(&in_cache);
                        throw;
                    }
                } catch (...) {
                    started_ = false;
                    throw;
                }
            } else {
                started_ = false;
                return false;
            }
        }
#endif
    }
};

template <forward_joinable_view V>
class join_forward_view {
   private:
    // usually a reference unless the join_forward_stage_view is an rvalue
    [[no_unique_address]] V v_;
    using I = decltype(std::declval<V>().forward_iter());
    [[no_unique_address]] join_forward_view_stage<I> stage_;

   public:
    template <forward_joinable_view V2>
    constexpr join_forward_view(wrapping_construct_t,
                                V2&& v) noexcept(std::is_nothrow_constructible_v<V, V2>)
        : v_(std::forward<V2>(v)), stage_{.i = v_.forward_iter()} {}

    constexpr decltype(auto) forward_iter() {
        return join_forward_iterator<I>(wrapping_construct, stage_);
    }
    constexpr decltype(auto) backward_iter() {
        return join_forward_sentinel<decltype(v_.backward_iter())>(wrapping_construct,
                                                                   v_.backward_iter());
    }
};

template <forward_joinable_view V2>
join_forward_view(wrapping_construct_t, V2&& v) -> join_forward_view<V2>;

}  // namespace impl

template <impl::forward_joinable_view V>
class join_forward_stage_view {
   private:
    [[no_unique_address]] V v_;

   public:
    template <impl::forward_joinable_view V2>
    constexpr join_forward_stage_view(wrapping_construct_t,
                                      V2&& v) noexcept(std::is_nothrow_constructible_v<V, V2>)
        : v_(std::forward<V2>(v)) {}

    constexpr auto stage() & { return impl::join_forward_view(wrapping_construct, v_); }
    constexpr auto stage() const& { return impl::join_forward_view(wrapping_construct, v_); }
    constexpr auto stage() && { return impl::join_forward_view(wrapping_construct, std::move(v_)); }
    constexpr auto stage() const&& {
        return impl::join_forward_view(wrapping_construct, std::move(v_));
    }
};

template <impl::forward_joinable_view V2>
join_forward_stage_view(wrapping_construct_t, V2&& v) -> join_forward_stage_view<V2>;

namespace impl {
struct join_forward_s_adaptor {
    template <forward_joinable_view V>
    constexpr DUALITY_STATIC_CALL auto operator()(V&& v) DUALITY_CONST_CALL {
        return join_forward_stage_view(wrapping_construct, std::forward<V>(v));
    }
};
struct join_forward_s {
    template <forward_joinable_view V>
    constexpr DUALITY_STATIC_CALL auto operator()(V&& v) DUALITY_CONST_CALL {
        return join_forward_stage_view(wrapping_construct, std::forward<V>(v));
    }
    constexpr DUALITY_STATIC_CALL auto operator()() DUALITY_CONST_CALL {
        return join_forward_s_adaptor{};
    }
};
}  // namespace impl

namespace stage_views {
constexpr inline impl::join_forward_s join_forward;
}

}  // namespace duality
