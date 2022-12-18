#pragma once

#include <cstddef>

namespace core {

int LastSystemError() noexcept;
void LastSystemErrorText(char* str, size_t size, int code) noexcept;
const char* LastSystemErrorText(int code) noexcept;

inline auto LastSystemErrorText() noexcept { return LastSystemErrorText(LastSystemError()); }

inline void LastSystemErrorText(char* str, size_t size) noexcept { LastSystemErrorText(str, size, LastSystemError()); }

}  // namespace core