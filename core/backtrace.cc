#include "backtrace.h"
#include "array_size.h"
#include "demangle.h"

#include <cstring>
#include <iostream>
#include <sstream>

#include <dlfcn.h>
#include <unwind.h>

namespace detail {

struct BackTraceContext {
  void** sym;
  size_t cnt;
  size_t size;
};

static auto Helper(struct _Unwind_Context* c, void* h) noexcept {
  auto bt = static_cast<BackTraceContext*>(h);

  if (bt->cnt != 0) {
    bt->sym[bt->cnt - 1] = reinterpret_cast<void*>(_Unwind_GetIP(c));
  }

  if (bt->cnt == bt->size) {
    return _URC_END_OF_STACK;
  }

  ++bt->cnt;
  return _URC_NO_REASON;
}

static inline void FastStrCopy(char* dst, const char* src, size_t size) noexcept {
  size_t left = size;

  if (left != 0) {
    while (--left != 0) {
      if ((*dst++ = *src++) == '\0') {
        break;
      }
    }
  }

  if (left == 0) {
    if (size != 0) {
      *dst = '\0';
    }
  }
}

static inline const auto CopyTo(const char* from, char* buf, size_t len) {
  FastStrCopy(buf, from, len);
  return buf;
}

}  // namespace detail

namespace core {

size_t BackTrace(void** p, size_t len) noexcept {
  if (len >= 1) {
    detail::BackTraceContext bt = {p, 0, len};
    _Unwind_Backtrace(detail::Helper, &bt);
    return bt.cnt - 1;
  }
  return 0;
}

ResolvedSymbol ResolveSymbol(void* sym, char* buffer, size_t len) noexcept {
  ResolvedSymbol ret = {"??", sym};
  Dl_info dli;
  memset(&dli, 0, sizeof(Dl_info));

  if (dladdr(sym, &dli) && dli.dli_sname) {
    ret.name = detail::CopyTo(CppDemangler().demangle(dli.dli_sname), buffer, len);
    ret.nearest_symbol = dli.dli_saddr;
  }

  return ret;
}

void FormatBackTrace(std::ostream& out, void* const* backtrace, size_t size) noexcept {
  char tmp[1024];
  memset(tmp, 0, sizeof(tmp));
  for (size_t i = 0; i < size; ++i) {
    ResolvedSymbol rs = ResolveSymbol(backtrace[i], tmp, sizeof(tmp));
    out << rs.name << "+"
        << (reinterpret_cast<ptrdiff_t>(backtrace[i]) - reinterpret_cast<ptrdiff_t>(rs.nearest_symbol)) << " ("
        << std::hex << reinterpret_cast<ptrdiff_t>(backtrace[i]) << std::dec << ")\n";
  }
}

void FormatBackTrace(std::ostream& out) noexcept {
  void* array[300];
  const size_t s = BackTrace(array, core_array_size(array));
  FormatBackTrace(out, array, s);
}

void PrintBackTrace() noexcept { FormatBackTrace(std::cerr); }

BackTrace::BackTrace() noexcept
    : size_(0) {}

void BackTrace::capture() noexcept { size_ = core::BackTrace(data_.data(), kCapacity_); }

void BackTrace::printTo(std::ostream& out) const noexcept { FormatBackTrace(out, data_.data(), size_); }

std::string BackTrace::toString() const noexcept {
  std::ostringstream ss;
  printTo(ss);
  return ss.str();
}

}  // namespace core