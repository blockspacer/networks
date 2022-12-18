#pragma once

#include <cstddef>

namespace core {

using AtExitFunc = void (*)(void*);
using TraditionalAtExitFunc = void (*)();

void AtExit(AtExitFunc func, void* ctx);
void AtExit(AtExitFunc func, void* ctx, size_t priority);

void AtExit(TraditionalAtExitFunc func);
void AtExit(TraditionalAtExitFunc func, size_t priority);

}  // namespace core