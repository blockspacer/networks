#pragma once

#define core_printf_like(n, m) __attribute__((format(__printf__, n, m)))

#define core_noinline __attribute__((noinline))
#define core_always_inline __attribute__((__always_inline__))

#define core_likely(x) __builtin_expect(!!(x), 1)
#define core_unlikely(x) __builtin_expect(!!(x), 0)

#define core_assume(c) ((c) ? (void)0 : __builtin_unreachable())
#define core_unreachable() __builtin_unreachable()

#define core_concat(x, y) core_concat_i(x, y)
#define core_concat_i(x, y) core_concat_ii(x, y)
#define core_concat_ii(x, y) x##y

#define core_as_string(x) core_as_string_i(x)
#define core_as_string_i(x) #x

#define core_unique_id(v) core_concat(v, __COUNTER__)

template <class T>
constexpr core_always_inline int core_ignore_result(T&&...) {  // NOLINT
  return 0;
}

#define core_maybe_unused [[maybe_unused]]
#define core_warn_unused_result [[nodiscard]]