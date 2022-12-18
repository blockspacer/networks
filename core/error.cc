#include "error.h"

#include <cerrno>
#include <cstring>

namespace core {

int LastSystemError() noexcept { return errno; }

void LastSystemErrorText(char* str, size_t size, int code) noexcept {
  char* msg = strerror_r(code, str, size);
  strncpy(str, msg, size);
}

const char* LastSystemErrorText(int code) noexcept { return strerror(code); }

}  // namespace core