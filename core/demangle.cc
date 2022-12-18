#include "demangle.h"

#include <cxxabi.h>

const char* core::CppDemangler::demangle(const char* name) noexcept {
  int status;
  buffer_.reset(__cxxabiv1::__cxa_demangle(name, nullptr, nullptr, &status));
  return status == 0 ? buffer_.get() : name;
}