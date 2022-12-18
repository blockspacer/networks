#include "dynlib.h"
#include "exception.h"
#include "guard.h"
#include "mutex.h"
#include "singleton.h"

class core::DynamicLibrary::Impl {
 public:
  inline ~Impl() {
    if (module_ && unloadable_) {
      dlclose(module_);
    }
  }

  inline void* SymOptional(const char* name) noexcept { return dlsym(module_, name); }

  inline void* Sym(const char* name) {
    void* symbol = SymOptional(name);

    if (symbol == nullptr) {
      core_throw Exception() << dlerror();
    }

    return symbol;
  }

  inline void SetUnloadable(bool unloadable) { unloadable_ = unloadable; }

  static inline Impl* SafeCreate(const char* path, int flags) {
    auto guard = Guard(*Singleton<CreateMutex>());
    return new Impl(path, flags);
  }

 private:
  inline Impl(const char* path, int flags)
      : module_(dlopen(path, flags))
      , unloadable_(false) {
    if (!module_) {
      core_throw Exception() << dlerror();
    }
  }

  class CreateMutex : public Mutex {};

 private:
  void* module_;
  bool unloadable_;
};

namespace core {

DynamicLibrary::DynamicLibrary() noexcept = default;

DynamicLibrary::~DynamicLibrary() = default;

DynamicLibrary::DynamicLibrary(const std::string_view path, int flags) { Open(path.data(), flags); }

void DynamicLibrary::Open(const char* path, int flags) { impl_.reset(Impl::SafeCreate(path, flags)); }

void DynamicLibrary::Close() noexcept { impl_.reset(nullptr); }

void* DynamicLibrary::SymOptional(const char* name) noexcept {
  if (!IsLoaded()) {
    return nullptr;
  }
  return impl_->SymOptional(name);
}

void* DynamicLibrary::Sym(const char* name) {
  if (!IsLoaded()) {
    core_throw Exception() << "dynamic library not loaded";
  }
  return impl_->Sym(name);
}

bool DynamicLibrary::IsLoaded() const noexcept { return (bool)impl_.get(); }

void DynamicLibrary::SetUnloadable(bool unloadable) { impl_->SetUnloadable(unloadable); }

}  // namespace core
