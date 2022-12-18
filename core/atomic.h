#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace core {

using AtomicType = intptr_t;
using Atomic = volatile AtomicType;

namespace atomics {

static inline void CompilerBarrier() noexcept { __asm__ __volatile__("" : : : "memory"); }

static inline auto Load(const Atomic& a) noexcept {
  AtomicType tmp;
  __atomic_load(&a, &tmp, __ATOMIC_ACQUIRE);
  return tmp;
}

static inline void Store(Atomic& a, AtomicType v) noexcept { __atomic_store(&a, &v, __ATOMIC_RELEASE); }

static inline auto Increment(Atomic& p) noexcept -> intptr_t { return __atomic_add_fetch(&p, 1, __ATOMIC_SEQ_CST); }

static inline auto GetAndIncrement(Atomic& p) noexcept -> intptr_t {
  return __atomic_fetch_add(&p, 1, __ATOMIC_SEQ_CST);
}

static inline auto Decrement(Atomic& p) noexcept -> intptr_t { return __atomic_sub_fetch(&p, 1, __ATOMIC_SEQ_CST); }

static inline auto GetAndDecrement(Atomic& p) noexcept -> intptr_t {
  return __atomic_fetch_sub(&p, 1, __ATOMIC_SEQ_CST);
}

static inline auto Add(Atomic& p, intptr_t v) noexcept -> intptr_t {
  return __atomic_add_fetch(&p, v, __ATOMIC_SEQ_CST);
}

static inline auto GetAndAdd(Atomic& p, intptr_t v) noexcept -> intptr_t {
  return __atomic_fetch_add(&p, v, __ATOMIC_SEQ_CST);
}

static inline auto Swap(Atomic* p, intptr_t v) noexcept {
  intptr_t ret;
  __atomic_exchange(p, &v, &ret, __ATOMIC_SEQ_CST);
  return ret;
}

static inline auto Cas(Atomic* a, intptr_t exchange, intptr_t compare) noexcept -> bool {
  return __atomic_compare_exchange(a, &compare, &exchange, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static inline auto GetAndCas(Atomic* a, intptr_t exchange, intptr_t compare) noexcept {
  __atomic_compare_exchange(a, &compare, &exchange, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  return compare;
}

static inline auto Or(Atomic& a, intptr_t b) noexcept -> intptr_t { return __atomic_or_fetch(&a, b, __ATOMIC_SEQ_CST); }

static inline auto Xor(Atomic& a, intptr_t b) noexcept -> intptr_t {
  return __atomic_xor_fetch(&a, b, __ATOMIC_SEQ_CST);
}

static inline auto And(Atomic& a, intptr_t b) noexcept -> intptr_t {
  return __atomic_and_fetch(&a, b, __ATOMIC_SEQ_CST);
}

static inline auto Sub(Atomic& a, AtomicType v) noexcept { return Add(a, -v); }

static inline auto GetAndSub(Atomic& a, AtomicType v) noexcept { return GetAndAdd(a, -v); }

static inline auto TryLock(Atomic* a) noexcept { return Cas(a, 1, 0); }

static inline auto TryAndTryLock(Atomic* a) noexcept { return (Load(*a) == 0) && TryLock(a); }

static inline void Unlock(Atomic* a) noexcept {
  CompilerBarrier();
  Store(*a, 0);
}

}  // namespace atomics

}  // namespace core

#include "atomic_ops.h"