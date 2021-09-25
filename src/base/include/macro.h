#ifndef __LJY_MACRO_H__
#define __LJY_MACRO_H__

#include <string.h>
#include <assert.h>
#include "log.h"
#include "util.h"

#if defined __GNUC__ || defined __llvm__
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率成立
#   define LJY_LIKELY(x)       __builtin_expect(!!(x), 1)
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率不成立
#   define LJY_UNLIKELY(x)     __builtin_expect(!!(x), 0)
#else
#   define LJY_LIKELY(x)      (x)
#   define LJY_UNLIKELY(x)      (x)
#endif

/// 断言宏封装
#define LJY_ASSERT(x) \
    if(LJY_UNLIKELY(!(x))) { \
        LJY_LOG_ERROR(LJY_LOG_ROOT()) << "ASSERTION: " #x \
            << "\nbacktrace:\n" \
            << ljy::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }

/// 断言宏封装
#define LJY_ASSERT2(x, w) \
    if(LJY_UNLIKELY(!(x))) { \
        LJY_LOG_ERROR(LJY_LOG_ROOT()) << "ASSERTION: " #x \
            << "\n" << w \
            << "\nbacktrace:\n" \
            << ljy::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }

#endif