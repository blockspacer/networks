#pragma once

#include "assert.h"
#include "fwd.h"
#include "ref_count.h"

namespace core {

template <class T, class D = Deleter>
using AtomicRefCount = RefCounted<T, AtomicCounter, D>;

template <class T>
inline void AssertTypeComplete() {
  static_assert(sizeof(T) != 0, "Type must be complete");
}

class Deleter {
 public:
  template <class T>
  static inline void destroy(T* t) noexcept {
    AssertTypeComplete<T>();
    delete (t);
  }

  static inline void destroy(std::nullptr_t) noexcept {}

  static void destroy(void* t) noexcept;
};

template <class Base, class T>
class PointerCommon {
 public:
  using ValueType = T;

  inline T* operator->() const noexcept {
    T* ptr = asT();
    core_assert(ptr);
    return ptr;
  }

  template <class C>
  inline bool operator==(const C& p) const noexcept {
    return (p == asT());
  }

  template <class C>
  inline bool operator!=(const C& p) const noexcept {
    return (p != asT());
  }

  inline explicit operator bool() const noexcept { return nullptr != asT(); }

 protected:
  inline T* asT() const noexcept { return (static_cast<const Base*>(this))->get(); }

  static inline T* doRelease(T*& t) noexcept {
    T* ret = t;
    t = nullptr;
    return ret;
  }
};

template <class Base, class T>
class PointerBase : public PointerCommon<Base, T> {
 public:
  inline T& operator*() const noexcept {
    core_assert(this->asT());
    return *(this->asT());
  }

  inline T& operator[](size_t n) const noexcept {
    core_assert(this->asT());
    return (this->asT())[n];
  }
};

template <class Base>
class PointerBase<Base, void> : public PointerCommon<Base, void> {};

template <class T>
class DefaultIntrusivePtrOps {
 public:
  static inline void ref(T* t) noexcept {
    core_assert(t);
    t->ref();
  }

  static inline void unRef(T* t) noexcept {
    core_assert(t);
    t->unRef();
  }

  static inline void decRef(T* t) noexcept {
    core_assert(t);
    t->decRef();
  }

  static inline long refCount(const T* t) noexcept {
    core_assert(t);
    return t->refCount();
  }
};

template <class T, class Ops>
class IntrusivePtr : public PointerBase<IntrusivePtr<T, Ops>, T> {
  friend class IntrusiveConstPtr<T, Ops>;

 public:
  inline IntrusivePtr(T* t = nullptr) noexcept
      : t_(t) {
    Ops();
    ref();
  }

  inline ~IntrusivePtr() { unRef(); }

  inline IntrusivePtr(const IntrusivePtr& p) noexcept
      : t_(p.t_) {
    ref();
  }

  template <class U>
  inline IntrusivePtr(const IntrusivePtr<U>& p,
                      std::enable_if_t<std::is_convertible<U*, T*>::value>* = nullptr) noexcept
      : t_(p.get()) {
    ref();
  }

  inline IntrusivePtr(IntrusivePtr&& p) noexcept
      : t_(nullptr) {
    swap(p);
  }

  inline IntrusivePtr& operator=(IntrusivePtr p) noexcept {
    p.swap(*this);
    return *this;
  }

  inline void reset(IntrusivePtr t) noexcept { swap(t); }

  inline void reset() noexcept { drop(); }

  inline T* get() const noexcept { return t_; }

  inline void swap(IntrusivePtr& r) noexcept { std::swap(t_, r.t_); }

  inline void drop() noexcept { IntrusivePtr(nullptr).swap(*this); }

  core_warn_unused_result inline T* release() const noexcept {
    T* res = t_;
    if (t_) {
      Ops::decRef(t_);
      t_ = nullptr;
    }
    return res;
  }

  inline long refCount() const noexcept { return t_ ? Ops::refCount(t_) : 0; }

 private:
  inline void ref() noexcept {
    if (t_) {
      Ops::ref(t_);
    }
  }

  inline void unRef() noexcept {
    if (t_) {
      Ops::unRef(t_);
    }
  }

 private:
  mutable T* t_;
};

template <class T, class Ops>
class IntrusiveConstPtr : public PointerBase<IntrusiveConstPtr<T, Ops>, const T> {
 public:
  inline IntrusiveConstPtr(T* t = nullptr) noexcept
      : t_(t) {
    Ops();
    ref();
  }

  inline ~IntrusiveConstPtr() { unRef(); }

  inline IntrusiveConstPtr(const IntrusiveConstPtr& p) noexcept
      : t_(p.t_) {
    ref();
  }

  inline IntrusiveConstPtr(IntrusiveConstPtr&& p) noexcept
      : t_(nullptr) {
    swap(p);
  }

  inline IntrusiveConstPtr(IntrusivePtr<T, Ops> p) noexcept
      : t_(nullptr) {
    std::swap(t_, p.T_);
  }

  template <class U>
  inline IntrusiveConstPtr(const IntrusiveConstPtr<U>& p,
                           std::enable_if_t<std::is_convertible<U*, T*>::value>* = nullptr) noexcept
      : t_(p.t_) {
    ref();
  }

  inline IntrusiveConstPtr& operator=(IntrusiveConstPtr p) noexcept {
    p.swap(*this);

    return *this;
  }

  inline void reset(IntrusiveConstPtr t) noexcept { swap(t); }

  inline void reset() noexcept { drop(); }

  inline const T* get() const noexcept { return t_; }

  inline void swap(IntrusiveConstPtr& r) noexcept { std::swap(t_, r.t_); }

  inline void drop() noexcept { IntrusiveConstPtr(nullptr).swap(*this); }

  core_warn_unused_result inline long refCount() const noexcept { return t_ ? Ops::refCount(t_) : 0; }

 private:
  inline void ref() noexcept {
    if (t_ != nullptr) {
      Ops::ref(t_);
    }
  }

  inline void unRef() noexcept {
    if (t_ != nullptr) {
      Ops::unRef(t_);
    }
  }

 private:
  template <class U, class O>
  friend class IntrusiveConstPtr;

 private:
  T* t_;
};

template <class T, class Ops = DefaultIntrusivePtrOps<T>, class... Args>
IntrusivePtr<T, Ops> MakeIntrusive(Args&&... args) {
  return new T{std::forward<Args>(args)...};
}

template <class T, class Ops = DefaultIntrusivePtrOps<T>, class... Args>
IntrusiveConstPtr<T, Ops> MakeIntrusiveConst(Args&&... args) {
  return new T{std::forward<Args>(args)...};
}

}  // namespace core