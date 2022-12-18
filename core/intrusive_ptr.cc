#include "intrusive_ptr.h"

#include <new>

void core::Deleter::destroy(void* t) noexcept { ::operator delete(t); }