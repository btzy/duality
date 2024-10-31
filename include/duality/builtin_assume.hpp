// This file is part of https://github.com/btzy/duality
#pragma once

// function that makes the compiler assume that the argument is true

namespace duality::impl {

// default implementation
#define ALWAYS_INLINE inline

// MSVC
#if defined(_MSC_VER)
#undef ALWAYS_INLINE
#define ALWAYS_INLINE inline __forceinline
#endif

// GCC or Clang
#if defined(__has_attribute)
#if __has_attribute(always_inline)
#undef ALWAYS_INLINE
#define ALWAYS_INLINE inline __attribute__((always_inline))
#endif
#endif

// Some compilers complain when we don't have a return statement for a function returning non-void,
// even if it is unreachable.
// default implementation
#define UNREACHABLE_RETURN_BEGIN
#define UNREACHABLE_RETURN_END

// MSVC
#if defined(_MSC_VER)
#undef UNREACHABLE_RETURN_BEGIN
#undef UNREACHABLE_RETURN_END
#define UNREACHABLE_RETURN_BEGIN __pragma(warning(push)) __pragma(warning(disable : 4716))
#define UNREACHABLE_RETURN_END __pragma(warning(pop))
#endif

// GCC
#if defined(__GNUC__)
#undef UNREACHABLE_RETURN_BEGIN
#undef UNREACHABLE_RETURN_END
#define UNREACHABLE_RETURN_BEGIN \
    _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic warning \"-Wreturn-type\"")
#define UNREACHABLE_RETURN_END _Pragma("GCC diagnostic pop")
#endif

// Clang
#if defined(__clang__)
#undef UNREACHABLE_RETURN_BEGIN
#undef UNREACHABLE_RETURN_END
#define UNREACHABLE_RETURN_BEGIN \
    _Pragma("clang diagnostic push") _Pragma("clang diagnostic warning \"-Wreturn-type\"")
#define UNREACHABLE_RETURN_END _Pragma("clang diagnostic pop")
#endif

// default implementation does nothing
#define FUNC \
    ALWAYS_INLINE void builtin_assume(bool b) {}

// MSVC supports __assume
#if defined(_MSC_VER)
#undef FUNC
#define FUNC                                    \
    ALWAYS_INLINE void builtin_assume(bool b) { \
        __assume(b);                            \
    }
#endif

// older GCC supports __builtin_unreachable
#if defined(__GNUC__)
#undef FUNC
#define FUNC                                    \
    ALWAYS_INLINE void builtin_assume(bool b) { \
        if (!b) __builtin_unreachable();        \
    }
#endif

// newer GCC supports assume attribute
#if defined(__has_attribute)
#if __has_attribute(assume)
#undef FUNC
#define FUNC                                    \
    ALWAYS_INLINE void builtin_assume(bool b) { \
        __attribute__((assume(b)));             \
    }
#endif
#endif

// Clang supports __builtin_assume
#if defined(__clang__)
#undef FUNC
#define FUNC                                    \
    ALWAYS_INLINE void builtin_assume(bool b) { \
        __builtin_assume(b);                    \
    }
#endif

// C++23 assume attribute
#if defined(__has_cpp_attribute)
#if __has_cpp_attribute(assume) >= 202207L
#undef FUNC
#define FUNC                                    \
    ALWAYS_INLINE void builtin_assume(bool b) { \
        [[assume(b)]];                          \
    }
#endif
#endif

FUNC;

#undef FUNC

// default implementation does nothing
#define FUNC \
    [[noreturn]] ALWAYS_INLINE void builtin_unreachable() {}

// MSVC supports __assume
#if defined(_MSC_VER)
#undef FUNC
#define FUNC                                                \
    [[noreturn]] ALWAYS_INLINE void builtin_unreachable() { \
        __assume(0);                                        \
    }
#endif

// GCC and Clang support __builtin_unreachable
#if defined(__GNUC__)
#undef FUNC
#define FUNC                                                \
    [[noreturn]] ALWAYS_INLINE void builtin_unreachable() { \
        __builtin_unreachable();                            \
    }
#endif

FUNC;

#undef FUNC

#undef ALWAYS_INLINE

}  // namespace duality::impl
