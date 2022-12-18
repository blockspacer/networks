#pragma once

#include "macro.h"

#include <array>
#include <cstddef>
#include <ostream>
#include <string>

namespace core {

size_t BackTrace(void** p, size_t len) noexcept;

struct ResolvedSymbol {
  const char* name;
  void* nearest_symbol;
};

ResolvedSymbol ResolveSymbol(void* sym, char* buffer, size_t len) noexcept;

void FormatBackTrace(std::ostream& out, void* const* backtrace, size_t size) noexcept;
void FormatBackTrace(std::ostream& out) noexcept;
void PrintBackTrace() noexcept;

class BackTrace {
 public:
  BackTrace() noexcept;

  void capture() noexcept;

  void printTo(std::ostream& out) const noexcept;

  core_warn_unused_result std::string toString() const noexcept;

 private:
  static const size_t kCapacity_ = 300;
  std::array<void*, kCapacity_> data_;
  size_t size_;
};

}  // namespace core