#pragma once

#include <memory>
#include <string_view>

#include <dlfcn.h>

#ifndef RTLD_GLOBAL
#define RTLD_GLOBAL (0)
#endif

#define DEFAULT_DLLOPEN_FLAGS (RTLD_NOW | RTLD_GLOBAL)

namespace core {

class DynamicLibrary {
 public:
  DynamicLibrary() noexcept;
  DynamicLibrary(const std::string_view path, int flags = DEFAULT_DLLOPEN_FLAGS);
  ~DynamicLibrary();

  void Open(const char* path, int flags = DEFAULT_DLLOPEN_FLAGS);
  void Close() noexcept;

  void* SymOptional(const char* name) noexcept;
  void* Sym(const char* name);

  [[nodiscard]] bool IsLoaded() const noexcept;
  void SetUnloadable(bool unloadable);

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace core