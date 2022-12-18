#pragma once

#include <cstdlib>
#include <memory>

namespace core {

class CppDemangler {
 public:
  const char* demangle(const char* name) noexcept;

 private:
  std::unique_ptr<char, void (*)(void*)> buffer_{nullptr, std::free};
};

}  // namespace core