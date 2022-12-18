#include "exception.h"
#include "demangle.h"

const char* core::Exception::what() const noexcept {
  message_ = buffer_.str();
  return message_.c_str();
}

const class core::BackTrace* core::Exception::backTrace() const noexcept { return nullptr; }

void core::SystemError::init() {
  append("(");
  append(LastSystemErrorText(status_));
  append(") ");
}

std::string core::CurrentExceptionMessage() {
  auto ptr = std::current_exception();
  if (ptr) {
    try {
      std::rethrow_exception(ptr);
    } catch (const Exception& e) {
      const auto* bt = e.backTrace();
      return (bt ? bt->toString() + "\n" : std::string()) + CppDemangler().demangle(typeid(e).name()) + "(" +
             std::string(e.what()) + ")";
    }
  }
  return "(NO EXCEPTION)";
}