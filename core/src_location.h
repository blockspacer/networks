#pragma once

#include <ostream>
#include <string_view>

namespace core {

struct SourceLocation {
  constexpr SourceLocation(const std::string_view f, int l) noexcept
      : file(f)
      , line(l) {}

  std::string_view file;
  int line;
};

}  // namespace core

#define core_source_location ::core::SourceLocation(__FILE__, __LINE__)
