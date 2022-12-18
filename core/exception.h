#pragma once

#include "backtrace.h"
#include "error.h"
#include "macro.h"
#include "src_location.h"

#include <exception>
#include <sstream>

template <class T>
struct RemoveCvRef {
  using Type = std::remove_cv_t<std::remove_reference_t<T>>;
};

template <class T>
using RemoveCvRefT = typename RemoveCvRef<T>::Type;

namespace core {

class Exception : public std::exception {
 public:
  Exception() = default;
  ~Exception() override = default;

  Exception(const Exception& other) { buffer_ << other.buffer_.str(); }

  const char* what() const noexcept override;

  virtual const class BackTrace* backTrace() const noexcept;

  template <class T>
  inline void append(const T& t) {
    buffer_ << t;
  }

 private:
  std::ostringstream buffer_;
  mutable std::string message_;
};

template <class T, class E>
static inline std::enable_if_t<std::is_base_of_v<Exception, RemoveCvRefT<E>>, E>&& operator<<(E&& exception,
                                                                                              const T& data) {
  exception.append(data);
  return std::forward<E>(exception);
}

template <class E>
static inline std::enable_if_t<std::is_base_of_v<Exception, RemoveCvRefT<E>>, E>&& operator+(const SourceLocation& sl,
                                                                                             E&& exception) {
  return std::forward<E>(exception << sl.file << ":" << sl.line << ": ");
}

class SystemError : public Exception {
 public:
  SystemError(int status)
      : status_(status) {
    init();
  }

  SystemError()
      : SystemError(LastSystemError()) {}

  auto status() const noexcept { return status_; }

 private:
  void init();

 private:
  int status_;
};

template <class T>
class WithBackTrace : public T {
 public:
  template <class... Args>
  inline WithBackTrace(Args&&... args)
      : T(std::forward<Args>(args)...) {
    bt_.capture();
  }

  core_warn_unused_result const class BackTrace* backTrace() const noexcept override { return &bt_; }

 private:
  class BackTrace bt_;
};

std::string CurrentExceptionMessage();

}  // namespace core

#define core_throw throw core_source_location +

#define core_ensure(cond, expr)   \
  do {                            \
    if (core_unlikely(!(cond))) { \
      core_throw expr;            \
    }                             \
  } while (false)
