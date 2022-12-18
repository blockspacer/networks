#include "assert.h"
#include "datetime.h"
#include "singleton.h"
#include "spinlock.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>

namespace core::detail {

struct PanicLock final : public AdaptiveLock {};

[[noreturn]] core_noinline void PanicImpl(int line, const char* func, const char* expr, const std::string_view file,
                                          const char* error_message, size_t message_size) noexcept;

}  // namespace core::detail

void core::detail::Panic(const char* file, int line, const char* function, const char* expr, const char* format, ...) {
  try {
    auto guard = Guard(*Singleton<PanicLock>());
    std::array<char, 1024> buffer;
    buffer.fill(0);

    va_list args;
    va_start(args, format);
    vsnprintf(buffer.data(), buffer.size(), format[0] == ' ' ? format + 1 : format, args);
    va_end(args);

    PanicImpl(line, function, expr, file, buffer.data(), strlen(buffer.data()));
  } catch (...) {
  }
  std::abort();
}

[[noreturn]] core_noinline void core::detail::PanicImpl(int line, const char* function, const char* expr,
                                                        const std::string_view file, const char* error_message,
                                                        size_t message_size) noexcept {
  try {
    std::string_view error(error_message, message_size);
    const auto now = Time::now();
    std::ostringstream out;

    if (expr) {
      out << "core_verify failed (" << now << "): " << error << std::endl;
    } else {
      out << "core_fail (" << now << "): " << error << std::endl;
    }

    out << " " << file << ":" << line << std::endl;

    if (expr) {
      out << " " << function << "(): requirement " << expr << " failed" << std::endl;
    } else {
      out << " " << function << "() failed" << std::endl;
    }

    std::cerr << out.str() << std::flush;
    std::abort();
  } catch (...) {
    std::abort();
  }
}
