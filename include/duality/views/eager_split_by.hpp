// This file is part of https://github.com/btzy/duality
#pragma once

#include <algorithm>
#include <concepts>
#include <utility>

#include <duality/builtin_assume.hpp>
#include <duality/core_view.hpp>
#include <duality/range.hpp>

/// Implementation for views::eager_split_by.  Splits the view at elements that satisfy the given
/// predicate.  Those elements matching the predicate are excluded from the yielded subviews.
///
/// E.g.:
/// eager_split_by({1, 3, 5, 3, 3, 7, 9}, [](int x) { return x == 3; }) -> {{1}, {5}, {}, {7, 9}}
///
/// views::eager_split_by accepts only multipass views.  The resultant subviews satisfy all view
/// concepts that the original view satisfies.  The resultant outer view satisfies
/// multipass_forward_view or multipass_backward_view if the original view supports them
/// respectively.  However, the resultant outer view satisfies neither sized_view nor any random
/// access concepts.
///
/// views::eager_split_by is iterator-preserving in the resultant subview iterator.  If the original
/// view is a multipass_forward_view, then the resultant subviews have the same forward iterator
/// type as the original view, and the backward iterator is the inverted forward iterator of the
/// original view. The opposite applies if the original view is a multipass_backward_view.  (If the
/// original view is a multipass_bidirectional_view, then the resultant subviews have the same
/// iterator types as the original view, due to the guarantees of multipass_bidirectional_view.)
///
/// This view differs from lazy_split_by, in that eager_split_by finds the other end of each
/// subrange before yielding it, but lazy_split_by does not.  This makes it possible for
/// lazy_split_by to operate on non-multipass views.

namespace duality {

namespace impl {
template <typename F, typename T>
concept eager_split_by_function = std::predicate<F, T&> && std::move_constructible<F>;
}

template <view V, impl::eager_split_by_function<view_element_type_t<V>> F>
class eager_split_by_view;

namespace impl {

// eager_split_by iterators must store the underlying view, so that it can determine the end of the
// range on its own.  This is required because to determine the end of the last subrange, we need to
// know the end of the range, but we may not be given a backward iterator (or be given a different
// backward iterator).

// Iterators that are actually iterators.  Templated on the underlying view (i.e. the view within
// the eager_split_by_view) and the predicate.  V might be cv-qualified.
template <multipass_iterator I,
          sentinel_for<I> S,
          eager_split_by_function<iterator_element_type_t<I>> F>
class eager_split_by_forward_iterator;
template <multipass_iterator I,
          sentinel_for<I> S,
          eager_split_by_function<iterator_element_type_t<I>> F>
class eager_split_by_backward_iterator;

// Non-iterator sentinels.
template <typename I>
class eager_split_by_forward_sentinel;
template <typename I>
class eager_split_by_backward_sentinel;

// Non-iterator sentinels special case for the beginning.
class eager_split_by_forward_begin_sentinel {};
class eager_split_by_backward_begin_sentinel {};

template <typename I>
class eager_split_by_forward_sentinel {
   private:
    template <multipass_iterator I2,
              sentinel_for<I2>,
              eager_split_by_function<iterator_element_type_t<I2>>>
    friend class eager_split_by_backward_iterator;
    // The source iterator, in the same direction, immediately before consuming the element for
    // which the predicate returns true (i.e. at the start of the next subrange).  Empty if this is
    // the start of the range.
    [[no_unique_address]] std::optional<I> i_before_;

    template <typename I2>
    constexpr eager_split_by_forward_sentinel(wrapping_construct_t, I2&& i_before) noexcept(
        std::is_nothrow_constructible_v<I, I2>)
        : i_before_(std::forward<I2>(i_before)) {}
};

template <typename I>
class eager_split_by_backward_sentinel {
   private:
    template <multipass_iterator I2,
              sentinel_for<I2>,
              eager_split_by_function<iterator_element_type_t<I2>>>
    friend class eager_split_by_forward_iterator;
    // The source iterator, in the same direction, immediately before consuming the element for
    // which the predicate returns true (i.e. at the start of the next subrange).  Empty if this is
    // the start of the range.
    [[no_unique_address]] std::optional<I> i_before_;

    template <typename I2>
    constexpr eager_split_by_backward_sentinel(wrapping_construct_t, I2&& i_before) noexcept(
        std::is_nothrow_constructible_v<I, I2>)
        : i_before_(std::forward<I2>(i_before)) {}
};

template <typename I>
struct make_eager_split_by_subrange_forward {
    using result_t = range<I, decltype(std::declval<I>().invert())>;
    template <typename Begin, typename End>
    static result_t make(Begin&& begin, End&& end) {
        return {std::forward<Begin>(begin), std::forward<End>(end)};
    }
    template <typename Begin, typename End>
    static optional<result_t> makeOptional(Begin&& begin, End&& end) {
        return {in_place, std::forward<Begin>(begin), std::forward<End>(end)};
    }
};
template <typename I>
struct make_eager_split_by_subrange_backward {
    using result_t = range<decltype(std::declval<I>().invert()), I>;
    template <typename Begin, typename End>
    static result_t make(Begin&& begin, End&& end) {
        return {std::forward<End>(end), std::forward<Begin>(begin)};
    }
    template <typename Begin, typename End>
    static optional<result_t> makeOptional(Begin&& begin, End&& end) {
        return {in_place, std::forward<End>(end), std::forward<Begin>(begin)};
    }
};

template <typename I,
          typename S,
          typename RangeMaker,
          eager_split_by_function<iterator_element_type_t<I>> F,
          template <view> typename SentinelT,
          typename BeginSentinelT>
class eager_split_by_iterator_base {
   protected:
    using element_type = typename RangeMaker::result_t;
    // The source iterator, in the same direction, immediately after consuming the element for
    // which the predicate returns true (i.e. at the start of the next subrange).
    [[no_unique_address]] std::optional<I> i_after_;
    // Backward iterator to end of the view.  (If this is a backward iterator, then this is actually
    // the beginning.)
    [[no_unique_address]] S i_backward_;
    // Pointer to the predicate to use.  Never null.  May be cv-qualified.
    F* f_;

    template <multipass_iterator I2, sentinel_for<I2> S2>
    constexpr eager_split_by_iterator_base(wrapping_construct_t,
                                           I2&& i_after,
                                           S2&& i_backward,
                                           F& f) noexcept(std::is_nothrow_constructible_v<I, I2> &&
                                                          std::is_nothrow_constructible_v<S, S2>)
        : i_after_(std::forward<I2>(i_after)), i_backward_(std::forward<S2>(i_backward)), f_(&f) {}

    using index_type = no_index_type_t;
    constexpr element_type next() {
        I begin = *i_after_;
        while (true) {
            I tmp = *i_after_;
            optional opt = i_after_->next(i_backward_);
            if (!opt) i_after_.reset();
            if (!opt || (*f_)(*opt)) return RangeMaker::make(begin, std::move(tmp).invert());
        }
    }
    template <sentinel_for<I> S2>
    constexpr optional<element_type> next(const SentinelT<S2>& end_i) {
        if (!i_after_) return nullopt;
        I begin = *i_after_;
        if (end_i.i_before_) {
            optional opt = i_after_->next(end_i.i_before_);
            if (!opt) return nullopt;
            if ((*f_)(*opt)) return RangeMaker::makeOptional(begin, begin.invert());
        }
        while (true) {
            I tmp = *i_after_;
            optional opt = i_after_->next(i_backward_);
            if (!opt) i_after_.reset();
            if (!opt || (*f_)(*opt))
                return RangeMaker::makeOptional(begin, std::move(tmp).invert());
        }
    }
    constexpr optional<element_type> next(const BeginSentinelT&) {
        if (!i_after_) return nullopt;
        I begin = *i_after_;
        while (true) {
            I tmp = *i_after_;
            optional opt = i_after_->next(i_backward_);
            if (!opt) i_after_.reset();
            if (!opt || (*f_)(*opt))
                return RangeMaker::makeOptional(begin, std::move(tmp).invert());
        }
    }
    constexpr void skip() {
        while (true) {
            optional opt = i_after_->next(i_backward_);
            if (!opt) i_after_.reset();
            if (!opt || (*f_)(*opt)) return;
        }
    }
    template <sentinel_for<I> S2>
    constexpr bool skip(const SentinelT<S2>& end_i) {
        if (!i_after_) return false;
        if (end_i.i_before_) {
            optional opt = i_after_->next(end_i.i_before_);
            if (!opt) return false;
            if ((*f_)(*opt)) return true;
        }
        while (true) {
            optional opt = i_after_->next(i_backward_);
            if (!opt) i_after_.reset();
            if (!opt || (*f_)(*opt)) return true;
        }
    }
    constexpr bool skip(const BeginSentinelT&) {
        if (!i_after_) return false;
        while (true) {
            optional opt = i_after_->next(i_backward_);
            if (!opt) i_after_.reset();
            if (!opt || (*f_)(*opt)) return true;
        }
    }
};

template <multipass_iterator I,
          sentinel_for<I> S,
          eager_split_by_function<iterator_element_type_t<I>> F>
class eager_split_by_forward_iterator
    : private eager_split_by_iterator_base<I,
                                           S,
                                           make_eager_split_by_subrange_forward<I>,
                                           F,
                                           eager_split_by_backward_sentinel,
                                           eager_split_by_backward_begin_sentinel> {
   private:
    template <view V2, eager_split_by_function<view_element_type_t<V2>>>
    friend class duality::eager_split_by_view;
    template <typename>
    friend class eager_split_by_backward_sentinel;

    template <typename I2, typename S2>
    constexpr eager_split_by_forward_iterator(
        wrapping_construct_t,
        I2&& i_after,
        S2&& i_backward,
        F& f) noexcept(std::is_nothrow_constructible_v<typename eager_split_by_forward_iterator::
                                                           eager_split_by_iterator_base,
                                                       wrapping_construct_t,
                                                       I2,
                                                       S2,
                                                       F&>)
        : eager_split_by_forward_iterator::eager_split_by_iterator_base(
              wrapping_construct,
              std::forward<I2>(i_after),
              std::forward<S2>(i_backward),
              f) {}

   public:
    using eager_split_by_forward_iterator::eager_split_by_iterator_base::next;
    using eager_split_by_forward_iterator::eager_split_by_iterator_base::skip;
    using typename eager_split_by_forward_iterator::eager_split_by_iterator_base::index_type;

    constexpr decltype(auto) invert() const {
        return this->i_after_
                   ? eager_split_by_backward_sentinel<decltype(std::declval<I>().invert())>(
                         wrapping_construct, this->i_after_->invert())
                   : eager_split_by_backward_sentinel<decltype(std::declval<I>().invert())>(
                         wrapping_construct, std::nullopt);
    }
};

template <multipass_iterator I,
          sentinel_for<I> S,
          eager_split_by_function<iterator_element_type_t<I>> F>
class eager_split_by_backward_iterator
    : private eager_split_by_iterator_base<I,
                                           S,
                                           make_eager_split_by_subrange_backward<I>,
                                           F,
                                           eager_split_by_forward_sentinel,
                                           eager_split_by_forward_begin_sentinel> {
   private:
    template <view V2, eager_split_by_function<view_element_type_t<V2>>>
    friend class duality::eager_split_by_view;
    template <typename>
    friend class eager_split_by_forward_sentinel;

    template <typename I2, typename S2>
    constexpr eager_split_by_backward_iterator(
        wrapping_construct_t,
        I2&& i_after,
        S2&& i_backward,
        F& f) noexcept(std::is_nothrow_constructible_v<typename eager_split_by_backward_iterator::
                                                           eager_split_by_iterator_base,
                                                       wrapping_construct_t,
                                                       I2,
                                                       S2,
                                                       F&>)
        : eager_split_by_backward_iterator::eager_split_by_iterator_base(
              wrapping_construct,
              std::forward<I2>(i_after),
              std::forward<S2>(i_backward),
              f) {}

   public:
    using eager_split_by_backward_iterator::eager_split_by_iterator_base::next;
    using eager_split_by_backward_iterator::eager_split_by_iterator_base::skip;
    using typename eager_split_by_backward_iterator::eager_split_by_iterator_base::index_type;

    constexpr decltype(auto) invert() const {
        return this->i_after_
                   ? eager_split_by_forward_sentinel<decltype(std::declval<I>().invert())>(
                         wrapping_construct, this->i_after_->invert())
                   : eager_split_by_forward_sentinel<decltype(std::declval<I>().invert())>(
                         wrapping_construct, std::nullopt);
    }
};

template <typename I,
          typename S,
          typename RangeMaker,
          eager_split_by_function<iterator_element_type_t<I>> F,
          template <typename, typename, typename> typename SentinelT>
// We actually want SentinelT to be this, but clang isn't happy with it
// template <reversible_iterator I2,
//           sentinel_for<I2>,
//           eager_split_by_function<iterator_element_type_t<I2>>> typename SentinelT
class eager_split_by_reversible_iterator_base {
   protected:
    using element_type = typename RangeMaker::result_t;
    // The source iterator, in the same direction, immediately after consuming the element for
    // which the predicate returns true (i.e. at the start of the next subrange).
    [[no_unique_address]] std::optional<I> i_after_;
    // The source iterator, in the same direction, immediately before consuming the element for
    // which the predicate returns true (i.e. at the end of the previous subrange).
    [[no_unique_address]] std::optional<I> i_before_;
    // Forward iterator to beginning of the view.  (If this is a backward iterator, then this is
    // actually the end.)
    [[no_unique_address]] I i_forward_;
    // Backward iterator to end of the view.  (If this is a backward iterator, then this is actually
    // the beginning.)
    [[no_unique_address]] S i_backward_;
    // Pointer to the predicate to use.  Never null.
    F* f_;

    template <typename I2, typename S2>
    constexpr eager_split_by_reversible_iterator_base(
        wrapping_construct_t,
        I2&& i_forward,
        S2&& i_backward,
        F& f) noexcept(std::is_nothrow_constructible_v<I, I2> &&
                       std::is_nothrow_constructible_v<S, S2>)
        : i_after_(std::in_place, std::forward<I2>(i_forward)),
          i_before_(),
          i_forward_(*i_after_),
          i_backward_(std::forward<S2>(i_backward)),
          f_(&f) {}

    template <typename I2, typename I3, typename I4, typename S2>
    constexpr eager_split_by_reversible_iterator_base(
        wrapping_construct_t,
        I2&& i_after,
        I3&& i_before,
        I4&& i_forward,
        S2&& i_backward,
        F& f) noexcept(std::is_nothrow_constructible_v<std::optional<I>, I2> &&
                       std::is_nothrow_constructible_v<std::optional<I>, I3> &&
                       std::is_nothrow_constructible_v<I, I4> &&
                       std::is_nothrow_constructible_v<S, S2>)
        : i_after_(std::forward<I2>(i_after)),
          i_before_(std::forward<I3>(i_before)),
          i_forward_(std::forward<I4>(i_forward)),
          i_backward_(std::forward<S2>(i_backward)),
          f_(&f) {}

    using index_type = no_index_type_t;
    constexpr element_type next() {
        I begin = *i_after_;
        while (true) {
            I tmp = *i_after_;
            optional opt = i_after_->next(i_backward_);
            if (!opt) i_after_.reset();
            if (!opt || (*f_)(*opt)) {
                return RangeMaker::make(begin, i_before_.emplace(std::move(tmp)).invert());
            }
        }
    }
    template <reversible_iterator I2,
              sentinel_for<I2> S2,
              eager_split_by_function<iterator_element_type_t<I2>> F2>
        requires sentinel_for<I2, I>
    constexpr optional<element_type> next(const SentinelT<I2, S2, F2>& end_i) {
        if (!i_after_) return nullopt;
        I begin = *i_after_;
        if (end_i.i_before_) {
            optional opt = i_after_->next(*end_i.i_before_);
            if (!opt) return nullopt;
            if ((*f_)(*opt))
                return RangeMaker::makeOptional(begin, i_before_.emplace(begin).invert());
        }
        while (true) {
            I tmp = *i_after_;
            optional opt = i_after_->next(i_backward_);
            if (!opt) i_after_.reset();
            if (!opt || (*f_)(*opt))
                return RangeMaker::makeOptional(begin, i_before_.emplace(std::move(tmp)).invert());
        }
    }
    constexpr void skip() {
        while (true) {
            I tmp = *i_after_;
            optional opt = i_after_->next(i_backward_);
            if (!opt) i_after_.reset();
            if (!opt || (*f_)(*opt)) {
                i_before_.emplace(std::move(tmp));
                return;
            }
        }
    }
    template <reversible_iterator I2,
              sentinel_for<I2> S2,
              eager_split_by_function<iterator_element_type_t<I2>> F2>
        requires sentinel_for<I2, I>
    constexpr bool skip(const SentinelT<I2, S2, F2>& end_i) {
        if (!i_after_) return false;
        I begin = *i_after_;
        if (end_i.i_before_) {
            optional opt = i_after_->next(*end_i.i_before_);
            if (!opt) return false;
            if ((*f_)(*opt)) {
                i_before_.emplace(begin);
                return true;
            }
        }
        while (true) {
            I tmp = *i_after_;
            optional opt = i_after_->next(i_backward_);
            if (!opt) i_after_.reset();
            if (!opt || (*f_)(*opt)) {
                i_before_.emplace(std::move(tmp));
                return true;
            }
        }
    }
};

template <reversible_iterator I,
          sentinel_for<I> S,
          eager_split_by_function<iterator_element_type_t<I>> F>
class eager_split_by_forward_iterator<I, S, F>
    : private eager_split_by_reversible_iterator_base<I,
                                                      S,
                                                      make_eager_split_by_subrange_forward<I>,
                                                      F,
                                                      eager_split_by_backward_iterator> {
   private:
    template <view V2, eager_split_by_function<view_element_type_t<V2>>>
    friend class duality::eager_split_by_view;
    template <multipass_iterator I2,
              sentinel_for<I2>,
              eager_split_by_function<iterator_element_type_t<I2>>>
    friend class eager_split_by_backward_iterator;
    template <typename I2,
              typename,
              typename,
              eager_split_by_function<iterator_element_type_t<I2>>,
              template <typename, typename, typename> typename>
    friend class eager_split_by_reversible_iterator_base;

    template <typename I2, typename S2>
    constexpr eager_split_by_forward_iterator(
        wrapping_construct_t,
        I2&& i_after,
        S2&& i_backward,
        F& f) noexcept(std::is_nothrow_constructible_v<typename eager_split_by_forward_iterator::
                                                           eager_split_by_reversible_iterator_base,
                                                       wrapping_construct_t,
                                                       I2,
                                                       S2,
                                                       F&>)
        : eager_split_by_forward_iterator::eager_split_by_reversible_iterator_base(
              wrapping_construct,
              std::forward<I2>(i_after),
              std::forward<S2>(i_backward),
              f) {}

    template <typename I2, typename I3, typename I4, typename S2>
    constexpr eager_split_by_forward_iterator(
        wrapping_construct_t,
        I2&& i_after,
        I3&& i_before,
        I4&& i_forward,
        S2&& i_backward,
        F& f) noexcept(std::is_nothrow_constructible_v<typename eager_split_by_forward_iterator::
                                                           eager_split_by_reversible_iterator_base,
                                                       wrapping_construct_t,
                                                       I2,
                                                       I3,
                                                       I4,
                                                       S2>)
        : eager_split_by_forward_iterator::eager_split_by_reversible_iterator_base(
              wrapping_construct,
              std::forward<I2>(i_after),
              std::forward<I3>(i_before),
              std::forward<I4>(i_forward),
              std::forward<S2>(i_backward),
              f) {}

   public:
    using eager_split_by_forward_iterator::eager_split_by_reversible_iterator_base::next;
    using eager_split_by_forward_iterator::eager_split_by_reversible_iterator_base::skip;
    using typename eager_split_by_forward_iterator::eager_split_by_reversible_iterator_base::
        index_type;

    constexpr decltype(auto) invert() const {
        builtin_assume(this->i_after_ || this->i_before_);
        return this->i_before_
                   ? (this->i_after_
                          ? eager_split_by_backward_iterator<S, I, F>(wrapping_construct,
                                                                      this->i_before_->invert(),
                                                                      this->i_after_->invert(),
                                                                      this->i_backward_,
                                                                      this->i_forward_,
                                                                      *this->f_)
                          : eager_split_by_backward_iterator<S, I, F>(wrapping_construct,
                                                                      this->i_before_->invert(),
                                                                      std::nullopt,
                                                                      this->i_backward_,
                                                                      this->i_forward_,
                                                                      *this->f_))
                   : (this->i_after_
                          ? eager_split_by_backward_iterator<S, I, F>(wrapping_construct,
                                                                      std::nullopt,
                                                                      this->i_after_->invert(),
                                                                      this->i_backward_,
                                                                      this->i_forward_,
                                                                      *this->f_)
                          : eager_split_by_backward_iterator<S, I, F>(wrapping_construct,
                                                                      std::nullopt,
                                                                      std::nullopt,
                                                                      this->i_backward_,
                                                                      this->i_forward_,
                                                                      *this->f_));
    }
};

template <reversible_iterator I,
          sentinel_for<I> S,
          eager_split_by_function<iterator_element_type_t<I>> F>
class eager_split_by_backward_iterator<I, S, F>
    : private eager_split_by_reversible_iterator_base<I,
                                                      S,
                                                      make_eager_split_by_subrange_backward<I>,
                                                      F,
                                                      eager_split_by_forward_iterator> {
   private:
    template <view V2, eager_split_by_function<view_element_type_t<V2>>>
    friend class duality::eager_split_by_view;
    template <multipass_iterator I2,
              sentinel_for<I2>,
              eager_split_by_function<iterator_element_type_t<I2>>>
    friend class eager_split_by_forward_iterator;
    template <typename I2,
              typename,
              typename,
              eager_split_by_function<iterator_element_type_t<I2>>,
              template <typename, typename, typename> typename>
    friend class eager_split_by_reversible_iterator_base;

    template <typename I2, typename S2>
    constexpr eager_split_by_backward_iterator(
        wrapping_construct_t,
        I2&& i_after,
        S2&& i_backward,
        F& f) noexcept(std::is_nothrow_constructible_v<typename eager_split_by_backward_iterator::
                                                           eager_split_by_reversible_iterator_base,
                                                       wrapping_construct_t,
                                                       I2,
                                                       S2,
                                                       F&>)
        : eager_split_by_backward_iterator::eager_split_by_reversible_iterator_base(
              wrapping_construct,
              std::forward<I2>(i_after),
              std::forward<S2>(i_backward),
              f) {}

    template <typename I2, typename I3, typename I4, typename S2>
    constexpr eager_split_by_backward_iterator(
        wrapping_construct_t,
        I2&& i_after,
        I3&& i_before,
        I4&& i_forward,
        S2&& i_backward,
        F& f) noexcept(std::is_nothrow_constructible_v<typename eager_split_by_backward_iterator::
                                                           eager_split_by_reversible_iterator_base,
                                                       wrapping_construct_t,
                                                       I2,
                                                       I3,
                                                       I4,
                                                       S2>)
        : eager_split_by_backward_iterator::eager_split_by_reversible_iterator_base(
              wrapping_construct,
              std::forward<I2>(i_after),
              std::forward<I3>(i_before),
              std::forward<I4>(i_forward),
              std::forward<S2>(i_backward),
              f) {}

   public:
    using eager_split_by_backward_iterator::eager_split_by_reversible_iterator_base::next;
    using eager_split_by_backward_iterator::eager_split_by_reversible_iterator_base::skip;
    using typename eager_split_by_backward_iterator::eager_split_by_reversible_iterator_base::
        index_type;

    constexpr decltype(auto) invert() const {
        builtin_assume(this->i_after_ || this->i_before_);
        return this->i_before_
                   ? (this->i_after_
                          ? eager_split_by_forward_iterator<S, I, F>(wrapping_construct,
                                                                     this->i_before_->invert(),
                                                                     this->i_after_->invert(),
                                                                     this->i_backward_,
                                                                     this->i_forward_,
                                                                     *this->f_)
                          : eager_split_by_forward_iterator<S, I, F>(wrapping_construct,
                                                                     this->i_before_->invert(),
                                                                     std::nullopt,
                                                                     this->i_backward_,
                                                                     this->i_forward_,
                                                                     *this->f_))
                   : (this->i_after_
                          ? eager_split_by_forward_iterator<S, I, F>(wrapping_construct,
                                                                     std::nullopt,
                                                                     this->i_after_->invert(),
                                                                     this->i_backward_,
                                                                     this->i_forward_,
                                                                     *this->f_)
                          : eager_split_by_forward_iterator<S, I, F>(wrapping_construct,
                                                                     std::nullopt,
                                                                     std::nullopt,
                                                                     this->i_backward_,
                                                                     this->i_forward_,
                                                                     *this->f_));
    }
};

}  // namespace impl

template <view V, impl::eager_split_by_function<view_element_type_t<V>> F>
class eager_split_by_view {
   private:
    [[no_unique_address]] V v_;
    [[no_unique_address]] F f_;

   public:
    template <view V2, impl::eager_split_by_function<view_element_type_t<V>> F2>
    constexpr eager_split_by_view(wrapping_construct_t, V2&& v, F2&& f) noexcept(
        std::is_nothrow_constructible_v<V, V2> && std::is_nothrow_constructible_v<F, F2>)
        : v_(std::forward<V2>(v)), f_(std::forward<F2>(f)) {}
    constexpr decltype(auto) forward_iter() {
        return impl::eager_split_by_forward_begin_sentinel{};
    }
    constexpr decltype(auto) forward_iter() const {
        return impl::eager_split_by_forward_begin_sentinel{};
    }
    constexpr decltype(auto) forward_iter()
        requires multipass_forward_view<V>
    {
        return impl::eager_split_by_forward_iterator<decltype(v_.forward_iter()),
                                                     decltype(v_.backward_iter()),
                                                     std::remove_reference_t<decltype((f_))>>(
            wrapping_construct, v_.forward_iter(), v_.backward_iter(), f_);
    }
    constexpr decltype(auto) forward_iter() const
        requires multipass_forward_view<V>
    {
        return impl::eager_split_by_forward_iterator<decltype(v_.forward_iter()),
                                                     decltype(v_.backward_iter()),
                                                     std::remove_reference_t<decltype((f_))>>(
            wrapping_construct, v_.forward_iter(), v_.backward_iter(), f_);
    }
    constexpr decltype(auto) backward_iter() {
        return impl::eager_split_by_backward_begin_sentinel{};
    }
    constexpr decltype(auto) backward_iter() const {
        return impl::eager_split_by_backward_begin_sentinel{};
    }
    constexpr decltype(auto) backward_iter()
        requires multipass_backward_view<V>
    {
        return impl::eager_split_by_backward_iterator<decltype(v_.backward_iter()),
                                                      decltype(v_.forward_iter()),
                                                      std::remove_reference_t<decltype((f_))>>(
            wrapping_construct, v_.backward_iter(), v_.forward_iter(), f_);
    }
    constexpr decltype(auto) backward_iter() const
        requires multipass_backward_view<V>
    {
        return impl::eager_split_by_backward_iterator<decltype(v_.backward_iter()),
                                                      decltype(v_.forward_iter()),
                                                      std::remove_reference_t<decltype((f_))>>(
            wrapping_construct, v_.backward_iter(), v_.forward_iter(), f_);
    }
    constexpr bool empty() const {
        // An eager_split_by view is never empty.  If the original view is empty, then
        // eager_split_by will yield a single subview (which is empty).
        return false;
    }
    constexpr auto size() const
        requires infinite_view<V>
    {
        return v_.size();
    }
};

template <view V2, impl::eager_split_by_function<view_element_type_t<V2>> F2>
eager_split_by_view(wrapping_construct_t, V2&& v, F2&& f) -> eager_split_by_view<V2, F2>;

namespace impl {

template <typename F>
struct eager_split_by_adaptor {
    template <view V>
        requires(multipass_forward_view<V> || multipass_backward_view<V>) &&
                impl::eager_split_by_function<F, view_element_type_t<V>>
    constexpr auto operator()(V&& v) const& {
        return eager_split_by_view(wrapping_construct, std::forward<V>(v), f);
    }
    template <view V>
        requires(multipass_forward_view<V> || multipass_backward_view<V>) &&
                impl::eager_split_by_function<F, view_element_type_t<V>>
    constexpr auto operator()(V&& v) && {
        return eager_split_by_view(wrapping_construct, std::forward<V>(v), std::forward<F>(f));
    }
    [[no_unique_address]] F f;
};
struct eager_split_by {
    template <view V, impl::eager_split_by_function<view_element_type_t<V>> F>
        requires multipass_forward_view<V> || multipass_backward_view<V>
    constexpr DUALITY_STATIC_CALL auto operator()(V&& v, F&& f) DUALITY_CONST_CALL {
        return eager_split_by_view(wrapping_construct, std::forward<V>(v), std::forward<F>(f));
    }
    template <typename F>
    constexpr DUALITY_STATIC_CALL auto operator()(F&& f) DUALITY_CONST_CALL {
        return eager_split_by_adaptor<F>{std::forward<F>(f)};
    }
};
}  // namespace impl

namespace views {
constexpr inline impl::eager_split_by eager_split_by;
}

}  // namespace duality
