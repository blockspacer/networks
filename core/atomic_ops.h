#pragma once

namespace core::atomics {

template <class T>
inline auto AsPtr(T volatile* target) noexcept {
  return reinterpret_cast<Atomic*>(target);
}

template <class T>
inline const auto AsPtr(T const volatile* target) noexcept {
  return reinterpret_cast<const Atomic*>(target);
}

template <class T>
struct AtomicTraits {
  enum {
    CASTABLE = std::is_integral<T>::value && sizeof(T) == sizeof(AtomicType) && !std::is_const<T>::value,
  };
};

template <class T, class TT>
using IfCastable = std::enable_if_t<AtomicTraits<T>::CASTABLE, TT>;

template <class T>
inline auto Load(T const volatile& target) noexcept -> IfCastable<T, T> {
  return static_cast<T>(Load(*AsPtr(&target)));
}

template <class T>
inline auto Store(T volatile& target, AtomicType value) noexcept -> IfCastable<T, void> {
  Store(*AsPtr(&target), value);
}

template <class T>
inline auto Increment(T volatile& target) noexcept -> IfCastable<T, T> {
  return static_cast<T>(Increment(*AsPtr(&target)));
}

template <class T>
inline auto GetAndIncrement(T volatile& target) noexcept -> IfCastable<T, T> {
  return static_cast<T>(GetAndIncrement(*AsPtr(&target)));
}

template <class T>
inline auto Decrement(T volatile& target) noexcept -> IfCastable<T, T> {
  return static_cast<T>(Decrement(*AsPtr(&target)));
}

template <class T>
inline auto GetAndDecrement(T volatile& target) noexcept -> IfCastable<T, T> {
  return static_cast<T>(GetAndDecrement(*AsPtr(&target)));
}

template <class T>
inline auto Add(T volatile& target, AtomicType value) noexcept -> IfCastable<T, T> {
  return static_cast<T>(Add(*AsPtr(&target), value));
}

template <class T>
inline auto GetAndAdd(T volatile& target, AtomicType value) noexcept -> IfCastable<T, T> {
  return static_cast<T>(GetAndAdd(*AsPtr(&target), value));
}

template <class T>
inline auto Sub(T volatile& target, AtomicType value) noexcept -> IfCastable<T, T> {
  return static_cast<T>(Sub(*AsPtr(&target), value));
}

template <class T>
inline auto GetAndSub(T volatile& target, AtomicType value) noexcept -> IfCastable<T, T> {
  return static_cast<T>(GetAndSub(*AsPtr(&target), value));
}

template <class T>
inline auto Swap(T volatile* target, AtomicType exchange) noexcept -> IfCastable<T, T> {
  return static_cast<T>(Swap(AsPtr(target), exchange));
}

template <class T>
inline auto Cas(T volatile* target, AtomicType exchange, AtomicType compare) noexcept -> IfCastable<T, bool> {
  return Cas(AsPtr(target), exchange, compare);
}

template <class T>
inline auto GetAndCas(T volatile* target, AtomicType exchange, AtomicType compare) noexcept -> IfCastable<T, T> {
  return static_cast<T>(GetAndCas(AsPtr(target), exchange, compare));
}

template <class T>
inline auto TryLock(T volatile* target) noexcept -> IfCastable<T, bool> {
  return TryLock(AsPtr(target));
}

template <class T>
inline auto TryAndTryLock(T volatile* target) noexcept -> IfCastable<T, bool> {
  return TryAndTryLock(AsPtr(target));
}

template <class T>
inline auto Unlock(T volatile* target) noexcept -> IfCastable<T, void> {
  Unlock(AsPtr(target));
}

template <class T>
inline auto Or(T volatile& target, AtomicType value) noexcept -> IfCastable<T, T> {
  return static_cast<T>(Or(*AsPtr(&target), value));
}

template <class T>
inline auto And(T volatile& target, AtomicType value) noexcept -> IfCastable<T, T> {
  return static_cast<T>(And(*AsPtr(&target), value));
}

template <class T>
inline auto Xor(T volatile& target, AtomicType value) noexcept -> IfCastable<T, T> {
  return static_cast<T>(Xor(*AsPtr(&target), value));
}

template <class T>
inline auto Load(T* const volatile& target) noexcept {
  return reinterpret_cast<T*>(Load(*AsPtr(&target)));
}

template <class T>
inline void Store(T* volatile& target, T* value) noexcept {
  Store(*AsPtr(&target), reinterpret_cast<AtomicType>(value));
}

template <class T>
inline void Store(T* volatile& target, std::nullptr_t) noexcept {
  Store(*AsPtr(&target), 0);
}

template <class T>
inline auto Swap(T* volatile* target, T* exchange) noexcept {
  return reinterpret_cast<T*>(Swap(AsPtr(target), reinterpret_cast<AtomicType>(exchange)));
}

template <class T>
inline auto Swap(T* volatile* target, std::nullptr_t) noexcept {
  return reinterpret_cast<T*>(Swap(AsPtr(target), 0));
}

template <class T>
inline auto Cas(T* volatile* target, T* exchange, T* compare) noexcept {
  return Cas(AsPtr(target), reinterpret_cast<AtomicType>(exchange), reinterpret_cast<AtomicType>(compare));
}

template <class T>
inline auto GetAndCas(T* volatile* target, T* exchange, T* compare) noexcept {
  return reinterpret_cast<T*>(
      GetAndCas(AsPtr(target), reinterpret_cast<AtomicType>(exchange), reinterpret_cast<AtomicType>(compare)));
}

template <class T>
inline auto Cas(T* volatile* target, T* exchange, std::nullptr_t) noexcept {
  return Cas(AsPtr(target), reinterpret_cast<AtomicType>(exchange), 0);
}

template <class T>
inline auto GetAndCas(T* volatile* target, T* exchange, std::nullptr_t) noexcept {
  return reinterpret_cast<T*>(GetAndCas(AsPtr(target), reinterpret_cast<AtomicType>(exchange), 0));
}

template <class T>
inline auto Cas(T* volatile* target, std::nullptr_t, T* compare) noexcept {
  return Cas(AsPtr(target), 0, reinterpret_cast<AtomicType>(compare));
}

template <class T>
inline auto GetAndCas(T* volatile* target, std::nullptr_t, T* compare) noexcept {
  return reinterpret_cast<T*>(GetAndCas(AsPtr(target), 0, reinterpret_cast<AtomicType>(compare)));
}

template <class T>
inline auto Cas(T* volatile* target, std::nullptr_t, std::nullptr_t) noexcept {
  return Cas(AsPtr(target), 0, 0);
}

template <class T>
inline auto GetAndCas(T* volatile* target, std::nullptr_t, std::nullptr_t) noexcept {
  return reinterpret_cast<T*>(GetAndCas(AsPtr(target), 0, 0));
}

}  // namespace core::atomics