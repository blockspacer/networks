#pragma once

#include "macro.h"
#include "noncopyable.h"

#include <memory>
#include <new>

#define core_thread(T) ::core::tls::Value<T>
#define core_static_thread(T) static ccore_thread(T)

#if defined(__clang__)
#define core_pod_thread(T) thread_local T
#define core_pod_static_thread(T) static thread_local T
#elif defined(__GNUC__)
#define core_pod_thread(T) __thread T
#define core_pod_static_thread(T) static __thread T
#endif

#if !defined(core_pod_thread) || !defined(core_pod_static_thread)
#define core_pod_thread(T) core_thread(T)
#define core_pod_static_thread(T) core_static_thread(T)
#else
#define core_have_fast_pod_tls
#endif

namespace core::detail {

void FillWithTrash(void* ptr, size_t len);

}  // namespace core::detail

namespace core::tls {

using Dtor = void (*)(void*);

class Key {
 public:
  Key(Dtor dtor);
  Key(Key&& key) noexcept;
  ~Key();

  [[nodiscard]] void* get() const;
  void set(void* ptr) const;

  static void cleanup() noexcept;

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

struct Cleaner {
  inline ~Cleaner() { Key::cleanup(); }
};

template <class T>
class Value final : public MoveOnly {
  struct Constructor {
    Constructor() noexcept = default;

    virtual ~Constructor() = default;

    virtual T* construct(void* ptr) const = 0;
  };

  struct DefaultConstructor final : public Constructor {
    T* construct(void* ptr) const override { return ::new (ptr) T; }
  };

  template <class U>
  class CopyConstructor final : public Constructor {
   public:
    inline CopyConstructor(const U& value)
        : value_(value) {}

    T* construct(void* ptr) const override { return ::new (ptr) T(value_); }

   private:
    U value_;
  };

 public:
  inline Value()
      : constructor_(new DefaultConstructor{})
      , key_(Dtor) {}

  template <class U>
  inline Value(const U& value)
      : constructor_(new CopyConstructor<U>{value})
      , key_(Dtor) {}

  template <class U>
  inline T& operator=(const U& value) {
    return get() = value;
  }

  inline operator const T&() const { return get(); }

  inline operator T&() { return get(); }

  inline const T& operator->() const { return get(); }

  inline T& operator->() { return get(); }

  inline const T* operator&() const { return getPtr(); }

  inline T* operator&() { return getPtr(); }

  inline T& get() const { return *getPtr(); }

  inline T* getPtr() const {
    T* value = static_cast<T*>(key_.get());
    if (!value) {
      auto mem = UniqueVoid(::operator new(sizeof(T)));
      std::unique_ptr<T> new_value(constructor_->construct(mem.get()));
      core_ignore_result(mem.release());
      key_.set(new_value.get());
      value = new_value.release();
    }
    return value;
  }

 private:
  static auto UniqueVoid(void* ptr) {
    return std::unique_ptr<void, void (*)(void* ptr)>{ptr, [](void* ptr) { ::operator delete(ptr); }};
  }

  static void Dtor(void* ptr) {
    auto mem = UniqueVoid(ptr);
    ((T*)ptr)->~T();
    detail::FillWithTrash(ptr, sizeof(T));
  }

 private:
  std::unique_ptr<Constructor> constructor_;
  Key key_;
};

}  // namespace core::tls

namespace core {

template <class T>
static inline T& TlsRef(tls::Value<T>& v) noexcept {
  return v;
}

template <class T>
static inline const T& TlsRef(const tls::Value<T>& v) noexcept {
  return v;
}

template <class T>
static inline T& TlsRef(T& v) noexcept {
  return v;
}

}  // namespace core
