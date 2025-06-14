#pragma once

#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#define MONO_ZERO_LEN_ARRAY 1

template <typename T>
using non_owning_ptr = T*;

#define DIAG_STR(s) #s
#define DIAG_JOINSTR(x, y) DIAG_STR(x##y)
#if defined(__clang__)
#define DIAG_DO_PRAGMA(x) _Pragma(#x)
#define DIAG_PRAGMA(compiler, x) DIAG_DO_PRAGMA(compiler diagnostic x)
#elif defined(_MSC_VER)
#define DIAG_DO_PRAGMA(x) __pragma(x)
#define DIAG_PRAGMA(compiler, x) DIAG_DO_PRAGMA(warning(x))
#else
#define DIAG_DO_PRAGMA(x) _Pragma(#x)
#define DIAG_PRAGMA(compiler, x) DIAG_DO_PRAGMA(compiler diagnostic x)
#endif
#if defined(__clang__)
#define DIAG_PUSH_PRAGMA DIAG_PRAGMA(clang, push)
#define DIAG_DISABLE_WARNING(gcc_unused, clang_option, msvc_unused)                                          \
	DIAG_PRAGMA(clang, ignored DIAG_JOINSTR(-W, clang_option))
#define DIAG_POP_PRAGMA DIAG_PRAGMA(clang, pop)
#elif defined(_MSC_VER)
#define DIAG_PUSH_PRAGMA DIAG_PRAGMA(msvc, push)
#define DIAG_DISABLE_WARNING(gcc_unused, clang_unused, msvc_errorcode)                                       \
	DIAG_PRAGMA(msvc, disable : msvc_errorcode)
#define DIAG_POP_PRAGMA DIAG_PRAGMA(msvc, pop)
#elif defined(__GNUC__)
#define DIAG_PUSH_PRAGMA DIAG_PRAGMA(GCC, push)
#define DIAG_DISABLE_WARNING(gcc_option, clang_unused, msvc_unused)                                          \
	DIAG_PRAGMA(GCC, ignored DIAG_JOINSTR(-W, gcc_option))
#define DIAG_POP_PRAGMA DIAG_PRAGMA(GCC, pop)
#endif

#define BEGIN_MONO_INCLUDE                                                                                   \
	DIAG_PUSH_PRAGMA                                                                                         \
	DIAG_DISABLE_WARNING(pedantic, pedantic, 4201)
#define END_MONO_INCLUDE DIAG_POP_PRAGMA
