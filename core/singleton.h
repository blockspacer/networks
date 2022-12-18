#pragma once

#include "at_exit.h"
#include "atomic.h"
#include "macro.h"

#include <new>
#include <utility>

namespace core {

template <class T>
struct SingletonTraits {
  static constexpr size_t kPriority = 65536;
};

namespace detail {

void FillWithTrash(void* ptr, size_t len);
void LockRecursive(Atomic& lock) noexcept;
void UnlockRecursive(Atomic& lock) noexcept;

template <class T>
void Destroyer(void* ptr) {
  ((T*)ptr)->~T();
  FillWithTrash(ptr, sizeof(T));
}

template <class T, size_t P, class... Args>
core_noinline T* SingletonBase(T*& ptr, Args&&... args) {
  static std::aligned_storage_t<sizeof(T), alignof(T)> buf;
  static Atomic lock;

  LockRecursive(lock);
  auto ret = atomics::Load(ptr);

  try {
    if (!ret) {
      ret = ::new (&buf) T(std::forward<Args>(args)...);

      try {
        AtExit(Destroyer<T>, ret, P);
      } catch (...) {
        Destroyer<T>(ret);
        throw;
      }

      atomics::Store(ptr, ret);
    }
  } catch (...) {
    UnlockRecursive(lock);
    throw;
  }

  UnlockRecursive(lock);
  return ret;
}

template <class T, size_t P, class... Args>
T* SingletonInt(Args&&... args) {
  static_assert(sizeof(T) < 32000, "use HugeSingleton instead");
  static T* ptr;
  auto ret = atomics::Load(ptr);

  if (core_unlikely(!ret)) {
    ret = SingletonBase<T, P>(ptr, std::forward<Args>(args)...);
  }

  return ret;
}

template <class T>
class Default {
 public:
  template <class... TArgs>
  inline Default(TArgs&&... args)
      : t_(std::forward<TArgs>(args)...) {}

  inline auto get() const noexcept { return &t_; }

 private:
  T t_;
};

template <class T>
struct HeapStore {
  template <class... TArgs>
  inline HeapStore(TArgs&&... args)
      : d(new T(std::forward<TArgs>(args)...)) {}

  inline ~HeapStore() { delete d; }

  T* d;
};

}  // namespace detail

template <class T, class... Args>
T* Singleton(Args&&... args) {
  return detail::SingletonInt<T, SingletonTraits<T>::kPriority>(std::forward<Args>(args)...);
}

template <class T, class... TArgs>
T* HugeSingleton(TArgs&&... args) {
  return Singleton<detail::HeapStore<T>>(std::forward<TArgs>(args)...)->d;
}

template <class T, size_t P, class... TArgs>
T* SingletonWithPriority(TArgs&&... args) {
  return detail::SingletonInt<T, P>(std::forward<TArgs>(args)...);
}

template <class T, size_t P, class... TArgs>
T* HugeSingletonWithPriority(TArgs&&... args) {
  return SingletonWithPriority<detail::HeapStore<T>, P>(std::forward<TArgs>(args)...)->d;
}

template <class T>
const T& Default() {
  return *(detail::SingletonInt<typename detail::Default<T>, SingletonTraits<T>::kPriority>()->get());
}

}  // namespace core